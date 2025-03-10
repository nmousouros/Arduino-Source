/*  Egg Autonomous State
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Compiler.h"
#include "CommonFramework/Tools/ErrorDumper.h"
#include "CommonFramework/ImageTools/ImageStats.h"
#include "CommonFramework/ImageTools/SolidColorTest.h"
#include "CommonFramework/Inference/InferenceException.h"
#include "CommonFramework/Inference/VisualInferenceRoutines.h"
#include "CommonFramework/Inference/FrozenImageDetector.h"
#include "CommonFramework/Inference/ImageMatchDetector.h"
#include "NintendoSwitch/Commands/NintendoSwitch_PushButtons.h"
#include "Pokemon/Pokemon_Notification.h"
#include "PokemonBDSP/PokemonBDSP_Settings.h"
#include "PokemonBDSP/Inference/PokemonBDSP_DialogDetector.h"
#include "PokemonBDSP/Inference/PokemonBDSP_SelectionArrow.h"
#include "PokemonBDSP/Inference/BoxSystem/PokemonBDSP_BoxShinyDetector.h"
#include "PokemonBDSP/Inference/BoxSystem/PokemonBDSP_IVCheckerReader.h"
#include "PokemonBDSP/Programs/PokemonBDSP_GameNavigation.h"
#include "PokemonBDSP/Programs/PokemonBDSP_BoxRelease.h"
#include "PokemonBDSP/Programs/PokemonBDSP_EncounterHandler.h"
#include "PokemonBDSP_EggRoutines.h"
#include "PokemonBDSP_EggFeedback.h"
#include "PokemonBDSP_EggAutonomousState.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{


EggAutonomousStats::EggAutonomousStats()
    : m_hatched(m_stats["Eggs Hatched"])
    , m_errors(m_stats["Errors"])
    , m_fetch_attempts(m_stats["Fetch Attempts"])
    , m_fetch_success(m_stats["Fetch Success"])
    , m_shinies(m_stats["Shinies"])
{
    m_display_order.emplace_back("Eggs Hatched");
    m_display_order.emplace_back("Errors", true);
    m_display_order.emplace_back("Fetch Attempts");
    m_display_order.emplace_back("Fetch Success");
    m_display_order.emplace_back("Shinies");
}




class EggReceivedDetector : public VisualInferenceCallback{
public:
    EggReceivedDetector(Color color = COLOR_RED)
        : m_fetched(false)
        , m_color(color)
        , m_box0(0.05, 0.10, 0.10, 0.80)
        , m_box1(0.87, 0.10, 0.10, 0.80)
    {}

    bool fetched() const{
        return m_fetched;
    }

    virtual void make_overlays(OverlaySet& items) const override{
        items.add(m_color, m_box0);
        items.add(m_color, m_box1);
    }
    virtual bool process_frame(
        const QImage& frame,
        std::chrono::system_clock::time_point timestamp
    ) override{
        ImageStats stats0 = image_stats(extract_box(frame, m_box0));
        ImageStats stats1 = image_stats(extract_box(frame, m_box1));
        if (!is_solid(stats0, {0.22951, 0.340853, 0.429638}, 0.15, 20)){
            return false;
        }
        if (!is_solid(stats1, {0.22951, 0.340853, 0.429638}, 0.15, 20)){
            return false;
        }
        m_fetched = true;
        return false;
    }

private:
    bool m_fetched;
    Color m_color;
    ImageFloatBox m_box0;
    ImageFloatBox m_box1;
};



EggAutonomousState::EggAutonomousState(
    ProgramEnvironment& env, ConsoleHandle& console,
    EggAutonomousStats& stats,
    EventNotificationOption& notification_nonshiny_keep,
    EventNotificationOption& notification_shiny,
    EventNotificationOption& notification_error,
    uint16_t scroll_to_read_delay,
    Language language,
    ShortcutDirection& shortcut,
    uint16_t travel_time_per_fetch,
    const EggHatchFilterOption& filters,
    uint8_t max_keepers
)
    : m_env(env), m_console(console)
    , m_stats(stats)
    , m_notification_nonshiny_keep(notification_nonshiny_keep)
    , m_notification_shiny(notification_shiny)
    , m_notification_error(notification_error)
    , m_scroll_to_read_delay(scroll_to_read_delay)
    , m_language(language)
    , m_shortcut(shortcut)
    , m_travel_time_per_fetch(travel_time_per_fetch)
    , m_filters(filters)
    , m_max_keepers(max_keepers)
{}

void EggAutonomousState::dump() const{
    std::string str = "Current State:\n";
    str += "    On Bike: ";
    str += (m_on_bike ? "Yes" : "No");
    str += "\n    Box Column: " + std::to_string(m_eggs_in_column);
    str += "\n    Party Eggs: " + std::to_string(m_eggs_in_party);
    str += "\n    Babies Saved: " + std::to_string(m_babies_saved);
    m_console.log(str);
}
void EggAutonomousState::set(const EggAutonomousState& state){
    m_on_bike = state.m_on_bike;
    m_eggs_in_column = state.m_eggs_in_column;
    m_eggs_in_party = state.m_eggs_in_party;
    m_babies_saved = state.m_babies_saved;
}

void EggAutonomousState::process_error(const std::string& name, const char* message){
    m_stats.m_errors++;
    m_console.log(message, COLOR_RED);
    QImage screen = m_console.video().snapshot();
    dump_image(
        m_console, m_env.program_info(),
        QString::fromStdString(name),
        screen
    );
    send_program_recoverable_error_notification(
        m_console,
        m_notification_error,
        m_env.program_info(),
        message,
        m_stats.to_str(),
        screen
    );
    PA_THROW_InferenceException(m_console, message);
}

void EggAutonomousState::process_shiny(const QImage& screen){
//    take_video(m_console);
    m_stats.m_shinies++;
    m_env.update_stats();
    send_encounter_notification(
        m_console,
        m_notification_noop,
        m_notification_shiny,
        m_env.program_info(),
        false, true, {{{}, ShinyType::UNKNOWN_SHINY}},
        screen,
        &m_stats
    );
}


void EggAutonomousState::withdraw_egg_column(){
    m_console.log("Withdrawing column from box to your party...");

    const uint16_t BOX_SCROLL_DELAY = GameSettings::instance().BOX_SCROLL_DELAY_0;
    const uint16_t BOX_PICKUP_DROP_DELAY = GameSettings::instance().BOX_PICKUP_DROP_DELAY;

    pbf_press_button(m_console, BUTTON_Y, 20, 50);
    pbf_press_button(m_console, BUTTON_Y, 20, 50);
    pickup_column(m_console);
    pbf_move_right_joystick(m_console, 0, 128, 20, BOX_SCROLL_DELAY);
    pbf_move_right_joystick(m_console, 128, 255, 20, BOX_SCROLL_DELAY);
    pbf_press_button(m_console, BUTTON_ZL, 20, BOX_PICKUP_DROP_DELAY);

    m_eggs_in_column = 0;
    m_eggs_in_party = 5;
}
bool EggAutonomousState::process_party(){
    m_console.log("Processing party...");

    const uint16_t BOX_SCROLL_DELAY = GameSettings::instance().BOX_SCROLL_DELAY_0;
//    std::chrono::milliseconds SCROLL_TO_READ_DELAY((uint64_t)m_scroll_to_read_delay * 1000 / TICKS_PER_SECOND);

    pbf_move_right_joystick(m_console, 0, 128, 20, BOX_SCROLL_DELAY);
    pbf_move_right_joystick(m_console, 128, 255, 20, BOX_SCROLL_DELAY);
    pbf_wait(m_console, m_scroll_to_read_delay);
    m_console.botbase().wait_for_all_requests();
//    m_env.wait_for(SCROLL_TO_READ_DELAY);

    BoxShinyDetector shiny_reader;
    IVCheckerReaderScope iv_reader(m_console, m_language);

    OverlaySet set(m_console);
    shiny_reader.make_overlays(set);

    //  Make sure the stats menu is up.
    QImage screen = m_console.video().snapshot();
    if (!shiny_reader.is_panel(screen)){
        process_error("StatsPanel", "Stats panel not detected.");
    }

    //  Run through the 5 hatchlings and release all the non-shinies.
    for (size_t c = 0; c < 5; c++){
        if (c != 0){
            pbf_move_right_joystick(m_console, 128, 0, 20, BOX_SCROLL_DELAY);
            pbf_move_right_joystick(m_console, 128, 255, 20, BOX_SCROLL_DELAY);
            pbf_wait(m_console, m_scroll_to_read_delay);
            m_console.botbase().wait_for_all_requests();
//            m_env.wait_for(SCROLL_TO_READ_DELAY);

            screen = m_console.video().snapshot();
            if (!shiny_reader.is_panel(screen)){
                process_error("StatsPanel", "Stats panel not detected.");
            }
        }
//        m_console.botbase().wait_for_all_requests();

        bool shiny = shiny_reader.detect(screen);
        if (shiny){
            m_console.log("Pokemon " + std::to_string(c) + " is shiny!", COLOR_BLUE);
            process_shiny(screen);
        }else{
            m_console.log("Pokemon " + std::to_string(c) + " is not shiny.", COLOR_PURPLE);
        }
        IVCheckerReader::Results IVs = iv_reader.read(m_console, screen);

        EggHatchAction action = m_filters.get_action(shiny, IVs);
        switch (action){
        case EggHatchAction::StopProgram:
            m_console.log("Program stop requested...");
            return true;
        case EggHatchAction::Keep:
            m_console.log("Moving Pokemon to keep box...", COLOR_BLUE);
            if (!shiny){
                send_encounter_notification(
                    m_console,
                    m_notification_nonshiny_keep,
                    m_notification_shiny,
                    m_env.program_info(),
                    false, false, {},
                    screen,
                    &m_stats
                );
            }
            pbf_press_button(m_console, BUTTON_ZL, 20, 105);
            pbf_press_button(m_console, BUTTON_ZL, 20, 105);
            pbf_move_right_joystick(m_console, 128, 0, 20, 105);
            pbf_move_right_joystick(m_console, 128, 0, 20, 105);
            pbf_move_right_joystick(m_console, 128, 0, 20, 105);
            pbf_move_right_joystick(m_console, 255, 128, 20, 105);
            pbf_press_button(m_console, BUTTON_ZL, 20, 230);
            pbf_move_right_joystick(m_console, 0, 128, 20, 105);
            pbf_move_right_joystick(m_console, 128, 0, 20, 105);
            pbf_move_right_joystick(m_console, 128, 0, 20, 105);
            pbf_press_button(m_console, BUTTON_ZL, 20, 105);
            pbf_press_button(m_console, BUTTON_B, 20, 230);
            pbf_press_button(m_console, BUTTON_R, 20, GameSettings::instance().BOX_CHANGE_DELAY_0);
            pbf_move_right_joystick(m_console, 0, 128, 20, 105);
            pbf_move_right_joystick(m_console, 128, 255, 20, 105);
            pbf_move_right_joystick(m_console, 128, 255, 20, 105);
            pbf_move_right_joystick(m_console, 128, 255, 20, 105);
            m_babies_saved++;
            if (m_babies_saved >= m_max_keepers){
                m_console.log("Max keepers reached. Stopping program...");
                return true;
            }
            break;
        case EggHatchAction::Release:
            m_console.log("Releasing Pokemon...", COLOR_PURPLE);
            release(m_env, m_console);
        }
    }

    pbf_move_right_joystick(m_console, 128, 0, 20, BOX_SCROLL_DELAY);
    pbf_move_right_joystick(m_console, 255, 128, 20, BOX_SCROLL_DELAY);
    return false;
}
bool EggAutonomousState::process_batch(){
    overworld_to_box(m_env, m_console);
    if (process_party()){
        return true;
    }
    withdraw_egg_column();
    box_to_overworld(m_env, m_console);
    return false;
}




void EggAutonomousState::fetch_egg(){
    if (m_eggs_in_column >= 5){
        process_error("FetchFullColumn", "Attempted to fetch an egg when column is full.");
    }

    //  Move to corner.
    m_console.botbase().wait_for_all_requests();
    m_console.log("Attempting to fetch an egg.");
    {
        ShortDialogWatcher dialog;
        int ret = run_until(
            m_env, m_console,
            [=](const BotBaseContext& context){
                pbf_move_left_joystick(context, 0, 255, 125, 0);
            },
            { &dialog }
        );
        if (ret >= 0){
            return;
        }
    }
    m_env.wait_for(std::chrono::milliseconds(200));

    m_console.log("Getting off bike.");
    if (m_on_bike){
        m_shortcut.run(m_console, 100);
        m_console.botbase().wait_for_all_requests();
        m_on_bike = false;
    }

    m_console.log("Going to daycare man.");
    {
        ShortDialogWatcher dialog;
        int ret = run_until(
            m_env, m_console,
            [=](const BotBaseContext& context){
                pbf_move_left_joystick(context, 0, 255, 30, 0);
                pbf_move_left_joystick(context, 128, 0, 35, 0);
                pbf_move_left_joystick(context, 255, 128, 60, 125);
            },
            { &dialog }
        );
        if (ret >= 0){
            return;
        }
    }

    //  Talk to daycare man.
    {
        ShortDialogWatcher dialog;
        int ret = run_until(
            m_env, m_console,
            [=](const BotBaseContext& context){
                pbf_press_button(context, BUTTON_ZL, 20, 230);
            },
            { &dialog }
        );
        if (ret < 0){
            process_error("DaycareMan", "Unable to find daycare man.");
        }
        m_console.log("Found daycare man!");
    }

//    QImage overworld = console.video().snapshot();

    {
        EggReceivedDetector received;
        run_until(
            m_env, m_console,
            [=](const BotBaseContext& context){
                pbf_mash_button(context, BUTTON_ZL, 500);
                pbf_mash_button(context, BUTTON_B, 500);
            },
            { &received }
        );
        m_stats.m_fetch_attempts++;
        if (received.fetched()){
            m_eggs_in_column++;
            m_stats.m_fetch_success++;
            m_console.log("Fetched an egg!", COLOR_BLUE);
        }else{
            m_console.log("No egg fetched.", COLOR_ORANGE);
        }
        m_env.update_stats();
    }

    m_console.log("Getting back on bike.");
    m_shortcut.run(m_console, 100);
    m_on_bike = true;
    pbf_move_left_joystick(m_console, 0, 255, 125, 0);
}
void EggAutonomousState::hatch_egg(){
    if (m_eggs_in_party == 0){
        process_error("HatchFullParty", "State Inconsistency: An egg started hatching when you have no eggs in your party.");
    }

    //  Hatch the egg.
    QImage overworld = m_console.video().snapshot();
//    overworld.save("test-0.png");
    {
        pbf_mash_button(m_console, BUTTON_B, 10 * TICKS_PER_SECOND);
        m_console.botbase().wait_for_all_requests();

        ShortDialogWatcher dialog;
        int ret = wait_until(
            m_env, m_console, std::chrono::seconds(30),
            { &dialog }
        );
        if (ret < 0){
            PA_THROW_InferenceException(m_console, "End of hatch not detected after 30 seconds.");
            process_error("NoHatchEnd", "End of hatch not detected after 30 seconds.");
        }
        m_console.log("Egg finished hatching.");
        m_stats.m_hatched++;
        m_env.update_stats();
        pbf_mash_button(m_console, BUTTON_B, 1 * TICKS_PER_SECOND);
    }

    //  Return to overworld.
    while (true){
        m_console.botbase().wait_for_all_requests();

        //  Wait for steady state and read it again.
        m_env.wait_for(std::chrono::milliseconds(200));
        ImageMatchWatcher matcher(overworld, {0.10, 0.10, 0.80, 0.60}, 100);
        SelectionArrowFinder arrow(m_console, {0.50, 0.60, 0.30, 0.20}, COLOR_GREEN);
        int ret = wait_until(
            m_env, m_console, std::chrono::seconds(30),
            {
                &matcher,
                &arrow,
            }
        );
        switch (ret){
        case 0:
            m_console.log("Returned to overworld.");
            m_eggs_in_party--;
            return;
        case 1:
            m_console.log("Detected prompt. Please turn off nicknaming.", COLOR_RED);
            m_stats.m_errors++;
            PA_THROW_StringException("Please turn off nicknaming.");
        default:
            m_console.log("Failed to detect overworld after 30 seconds. Did day/night change?", COLOR_RED);
//            pbf_mash_button(console, BUTTON_ZL, 30 * TICKS_PER_SECOND);
            return;
        }
    }
}

void EggAutonomousState::hatch_rest_of_party(){
    m_console.log("Hatching rest of party without fetching...");
    while (m_eggs_in_party > 0){
        dump();
        ShortDialogWatcher dialog;
        FrozenImageDetector frozen(COLOR_CYAN, {0, 0, 1, 0.5}, std::chrono::seconds(60), 20);
        int ret = run_until(
            m_env, m_console,
            [&](const BotBaseContext& context){
                egg_spin(context, 480 * TICKS_PER_SECOND);
//                egg_spin(context, 5 * TICKS_PER_SECOND);
            },
            {
                &dialog,
                &frozen,
            }
        );
        switch (ret){
        case 0:
            m_console.log("Egg is hatching!");
            m_console.botbase().wait_for_all_requests();
            hatch_egg();
            break;
        case 1:
            process_error("FrozenScreen", "Frozen screen detected. Possible game crash.");
        default:
            process_error("NoHatch", "No hatch detected after 8 minutes of spinning.");
        }

    }
}
void EggAutonomousState::spin_until_fetch_or_hatch(){
    m_console.botbase().wait_for_all_requests();
    m_console.log("Looking for more eggs...");
    ShortDialogWatcher dialog;
    int ret = run_until(
        m_env, m_console,
        [&](const BotBaseContext& context){
            egg_spin(context, m_travel_time_per_fetch);
        },
        { &dialog }
    );
    m_env.wait_for(std::chrono::milliseconds(200));
    if (ret < 0){
//        m_console.log("Attempting to fetch an egg.");
        fetch_egg();
    }else{
//        m_console.log("Egg is hatching!");
//        hatch_egg();
    }
}

bool EggAutonomousState::overworld_detect_and_run_state(){
    ShortDialogWatcher dialog;
    m_console.botbase().wait_for_all_requests();
    m_env.wait_for(std::chrono::milliseconds(200));
    dump();

    //  Egg is hatching. Handle that now.
    QImage screen = m_console.video().snapshot();
    if (dialog.detect(screen)){
        m_console.log("Egg is hatching!");
        hatch_egg();
        return false;
    }

    //  Need more eggs.
    if (m_eggs_in_column < 5){
        spin_until_fetch_or_hatch();
        return false;
    }

    //  More eggs to hatch.
    if (m_eggs_in_party > 0){
        hatch_rest_of_party();
        return false;
    }

    //  Done with batch.
    return true;
}




}
}
}
