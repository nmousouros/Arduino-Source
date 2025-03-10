/*  Pokedex Recommendation Finder
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "CommonFramework/Tools/StatsTracking.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "CommonFramework/Inference/ImageTools.h"
#include "CommonFramework/OCR/OCR_RawOCR.h"
#include "CommonFramework/OCR/OCR_Filtering.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Device.h"
#include "NintendoSwitch/Commands/NintendoSwitch_PushButtons.h"
#include "Pokemon/Resources/Pokemon_PokemonSlugs.h"
#include "Pokemon/Options/Pokemon_NameSelectWidget.h"
#include "Pokemon/Inference/Pokemon_NameReader.h"
#include "PokemonSwSh/PokemonSwSh_Settings.h"
#include "PokemonSwSh/Commands/PokemonSwSh_Commands_GameEntry.h"
#include "PokemonSwSh/Commands/PokemonSwSh_Commands_DateSpam.h"
#include "PokemonSwSh_DexRecFinder.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{


DexRecFinder_Descriptor::DexRecFinder_Descriptor()
    : RunnableSwitchProgramDescriptor(
        "PokemonSwSh:DexRecFinder",
        STRING_POKEMON + " SwSh", "Dex Rec Finder",
        "ComputerControl/blob/master/Wiki/Programs/PokemonSwSh/DexRecFinder.md",
        "Search for a " + STRING_POKEDEX + " recommendation by date-spamming.",
        FeedbackType::OPTIONAL_,
        PABotBaseLevel::PABOTBASE_12KB
    )
{}



class DexRecExclusion : public EditableTableRow{
public:
    operator const std::string&() const{ return m_slug; }

    virtual void load_json(const QJsonValue& json) override{
        QString value = json.toString();
        m_slug = value.toStdString();
    }
    virtual QJsonValue to_json() const override{
        return QString::fromStdString(m_slug);
    }

    virtual std::unique_ptr<EditableTableRow> clone() const override{
        return std::unique_ptr<EditableTableRow>(new DexRecExclusion(*this));
    }
    virtual std::vector<QWidget*> make_widgets(QWidget& parent) override{
        using namespace Pokemon;
        NameSelectWidget* box = new NameSelectWidget(parent, NATIONAL_DEX_SLUGS(), m_slug);
        box->connect(
            box, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            box, [&, box](int index){
                m_slug = box->slug();
            }
        );
        return {box};
    }

private:
    std::string m_slug;
};

class DexRecExclusionFactory : public EditableTableFactory{
public:
    virtual QStringList make_header() const override{
        QStringList list;
        list << STRING_POKEMON;
        return list;
    }
    virtual std::unique_ptr<EditableTableRow> make_row() const override{
        return std::unique_ptr<EditableTableRow>(new DexRecExclusion());
    }

    static const DexRecExclusionFactory& instance(){
        static DexRecExclusionFactory factory;
        return factory;
    }
};


DexRecFilters::DexRecFilters()
    : GroupOption("Stop Automatically (requires video feedback)", true, true)
    , LANGUAGE(
        "<b>Game Language:</b><br>This needs to be set correctly for stop filters to work correctly.",
        PokemonNameReader::instance().languages(), true
    )
    , DESIRED(
        "<b>Desired " + STRING_POKEMON + ":</b><br>Stop when it finds this " + STRING_POKEMON + ". Requires the language be set.",
        "Pokemon/Pokedex/Pokedex-National.json"
    )
    , EXCLUSIONS(
        "<b>Exclusions:</b><br>Do not stop on these " + STRING_POKEMON + " even if the desired " + STRING_POKEMON + " is found. "
        "Use this to avoid dex recs that include other " + STRING_POKEMON + " in the spawn pool you don't want.",
        DexRecExclusionFactory::instance(), true
    )
{
    PA_ADD_OPTION(LANGUAGE);
    PA_ADD_OPTION(DESIRED);
    PA_ADD_OPTION(EXCLUSIONS);
}



DexRecFinder::DexRecFinder(const DexRecFinder_Descriptor& descriptor)
    : SingleSwitchProgramInstance(descriptor)
    , GO_HOME_WHEN_DONE(false)
    , VIEW_TIME(
        "<b>View Time:</b><br>View the " + STRING_POKEDEX + " for this long before continuing.",
        "2 * TICKS_PER_SECOND"
    )
    , NOTIFICATION_PROGRAM_FINISH("Program Finished", true, true, ImageAttachmentMode::JPG)
    , NOTIFICATIONS({
        &NOTIFICATION_PROGRAM_FINISH,
        &NOTIFICATION_ERROR_FATAL,
    })
    , m_advanced_options(
        "<font size=4><b>Advanced Options:</b> You should not need to touch anything below here.</font>"
    )
    , ENTER_POKEDEX_TIME(
        "<b>Enter " + STRING_POKEDEX + " Time:</b><br>Wait this long for the " + STRING_POKEDEX + " to open.",
        "3 * TICKS_PER_SECOND"
    )
    , BACK_OUT_TIME(
        "<b>Back Out Time:</b><br>Mash B for this long to return to the overworld.",
        "3 * TICKS_PER_SECOND"
    )
{
    PA_ADD_OPTION(START_IN_GRIP_MENU);
    PA_ADD_OPTION(GO_HOME_WHEN_DONE);

    PA_ADD_OPTION(FILTERS);
    PA_ADD_OPTION(VIEW_TIME);
    PA_ADD_OPTION(NOTIFICATIONS);

    PA_ADD_STATIC(m_advanced_options);
    PA_ADD_OPTION(ENTER_POKEDEX_TIME);
    PA_ADD_OPTION(BACK_OUT_TIME);
}


struct DexRecFinder::Stats : public StatsTracker{
    Stats()
        : attempts(m_stats["Attempts"])
        , errors(m_stats["Read Errors"])
        , excluded(m_stats["Excluded"])
        , matches(m_stats["Matches"])
    {
        m_display_order.emplace_back(Stat("Attempts"));
        m_display_order.emplace_back(Stat("Read Errors"));
        m_display_order.emplace_back(Stat("Excluded"));
        m_display_order.emplace_back(Stat("Matches"));
    }

    std::atomic<uint64_t>& attempts;
    std::atomic<uint64_t>& errors;
    std::atomic<uint64_t>& excluded;
    std::atomic<uint64_t>& matches;
};
std::unique_ptr<StatsTracker> DexRecFinder::make_stats() const{
    return std::unique_ptr<StatsTracker>(new Stats());
}


void DexRecFinder::read_line(
    bool& found,
    bool& excluded,
    bool& bad_read,
    Logger& logger,
    Language language,
    const QImage& frame,
    const ImageFloatBox& box,
    const std::set<std::string>& desired,
    const std::set<std::string>& exclusions
){
    QImage image = extract_box(frame, box);
    OCR::make_OCR_filter(image).apply(image);

    OCR::StringMatchResult result = PokemonNameReader::instance().read_substring(logger, language, image);
    if (result.results.empty()){
        bad_read = true;
        return;
    }
    for (const auto& hit : result.results){
        if (desired.find(hit.second.token) != desired.end()){
            found = true;
        }
        if (exclusions.find(hit.second.token) != exclusions.end()){
            excluded = true;
        }
    }
}

void DexRecFinder::program(SingleSwitchProgramEnvironment& env){
    if (START_IN_GRIP_MENU){
        grip_menu_connect_go_home(env.console);
    }else{
        pbf_press_button(env.console, BUTTON_B, 5, 5);
        pbf_press_button(env.console, BUTTON_HOME, 10, GameSettings::instance().GAME_TO_HOME_DELAY_SAFE);
    }

    std::set<std::string> desired;
    desired.insert(FILTERS.DESIRED.slug());

    std::set<std::string> exclusions;
    for (size_t c = 0; c < FILTERS.EXCLUSIONS.size(); c++){
        exclusions.insert(static_cast<const DexRecExclusion&>(FILTERS.EXCLUSIONS[c]));
    }

    Stats& stats = env.stats<Stats>();

    while (true){
        home_to_date_time(env.console, true, true);
        neutral_date_skip(env.console);
        settings_to_enter_game(env.console, true);
        pbf_mash_button(env.console, BUTTON_B, 90);
        pbf_press_button(env.console, BUTTON_X, 20, GameSettings::instance().OVERWORLD_TO_MENU_DELAY - 20);

        if (FILTERS.enabled()){
            env.console.botbase().wait_for_all_requests();
            InferenceBoxScope box0(env.console, ImageFloatBox(0.75, 0.531 + 0 * 0.1115, 0.18, 0.059));
            InferenceBoxScope box1(env.console, ImageFloatBox(0.75, 0.531 + 1 * 0.1115, 0.18, 0.059));
            InferenceBoxScope box2(env.console, ImageFloatBox(0.75, 0.531 + 2 * 0.1115, 0.18, 0.059));
            InferenceBoxScope box3(env.console, ImageFloatBox(0.75, 0.531 + 3 * 0.1115, 0.18, 0.059));
            pbf_press_button(env.console, BUTTON_A, 10, ENTER_POKEDEX_TIME);
            env.console.botbase().wait_for_all_requests();

            QImage frame = env.console.video().snapshot();
            bool found = false;
            bool excluded = false;
            bool bad_read = false;
            if (frame.isNull()){
                bad_read = true;
            }else{
                read_line(found, excluded, bad_read, env.console, FILTERS.LANGUAGE, frame, box0, desired, exclusions);
                read_line(found, excluded, bad_read, env.console, FILTERS.LANGUAGE, frame, box1, desired, exclusions);
                read_line(found, excluded, bad_read, env.console, FILTERS.LANGUAGE, frame, box2, desired, exclusions);
                read_line(found, excluded, bad_read, env.console, FILTERS.LANGUAGE, frame, box3, desired, exclusions);
            }

            stats.attempts++;
            if (found){
                if (excluded){
                    env.log("Found desired, but contains exclusion.", COLOR_BLUE);
                    stats.excluded++;
                }else{
                    env.log("Found a match!", COLOR_BLUE);
                    stats.matches++;
                    break;
                }
            }
            if (bad_read){
                env.log("Read Errors. Pausing for user to see.", COLOR_RED);
                stats.errors++;
                pbf_wait(env.console, VIEW_TIME);
            }
        }else{
            stats.attempts++;
//            stats.errors++;
            pbf_press_button(env.console, BUTTON_A, 10, ENTER_POKEDEX_TIME);
            pbf_wait(env.console, VIEW_TIME);
        }
        env.update_stats();

        pbf_mash_button(env.console, BUTTON_B, BACK_OUT_TIME);
        pbf_press_button(env.console, BUTTON_HOME, 10, GameSettings::instance().GAME_TO_HOME_DELAY_SAFE);
    }

    env.update_stats();
    send_program_finished_notification(
        env.logger(), NOTIFICATION_PROGRAM_FINISH,
        env.program_info(),
        "Found a match!",
        stats.to_str(),
        env.console.video().snapshot(), false
    );
    GO_HOME_WHEN_DONE.run_end_of_program(env.console);
}



}
}
}
