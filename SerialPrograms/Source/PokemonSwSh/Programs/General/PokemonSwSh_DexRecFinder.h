/*  Pokedex Recommendation Finder
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonSwSh_DexRecFinder_H
#define PokemonAutomation_PokemonSwSh_DexRecFinder_H

#include "CommonFramework/Options/StaticTextOption.h"
#include "CommonFramework/Options/ScreenshotFormatOption.h"
#include "CommonFramework/Options/EditableTableOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "CommonFramework/OCR/OCR_LanguageOptionOCR.h"
#include "Pokemon/Options/Pokemon_NameSelectOption.h"
#include "Pokemon/Inference/Pokemon_NameReader.h"
#include "NintendoSwitch/Options/GoHomeWhenDoneOption.h"
#include "NintendoSwitch/Options/TimeExpressionOption.h"
#include "NintendoSwitch/Options/StartInGripMenuOption.h"
#include "NintendoSwitch/Framework/NintendoSwitch_SingleSwitchProgram.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{
using namespace Pokemon;


class DexRecFinder_Descriptor : public RunnableSwitchProgramDescriptor{
public:
    DexRecFinder_Descriptor();
};



class DexRecFilters : public GroupOption{
public:
    DexRecFilters();

    OCR::LanguageOCR LANGUAGE;
    PokemonNameSelect DESIRED;
    EditableTableOption EXCLUSIONS;
};


class DexRecFinder : public SingleSwitchProgramInstance{
public:
    DexRecFinder(const DexRecFinder_Descriptor& descriptor);

    virtual std::unique_ptr<StatsTracker> make_stats() const override;
    virtual void program(SingleSwitchProgramEnvironment& env) override;

private:
    void read_line(
        bool& found,
        bool& excluded,
        bool& bad_read,
        Logger& logger,
        Language language,
        const QImage& frame,
        const ImageFloatBox& box,
        const std::set<std::string>& desired,
        const std::set<std::string>& exclusions
    );

private:
    struct Stats;

private:
    StartInGripOrGameOption START_IN_GRIP_MENU;
    GoHomeWhenDoneOption GO_HOME_WHEN_DONE;

    DexRecFilters FILTERS;
    TimeExpressionOption<uint16_t> VIEW_TIME;

    EventNotificationOption NOTIFICATION_PROGRAM_FINISH;
    EventNotificationsOption NOTIFICATIONS;

    SectionDividerOption m_advanced_options;
    TimeExpressionOption<uint16_t> ENTER_POKEDEX_TIME;
    TimeExpressionOption<uint16_t> BACK_OUT_TIME;
};

}
}
}
#endif



