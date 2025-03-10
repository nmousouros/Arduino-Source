/*  Egg Autonomous
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonBDSP_EggAutonomous_H
#define PokemonAutomation_PokemonBDSP_EggAutonomous_H

#include "CommonFramework/Options/StaticTextOption.h"
#include "CommonFramework/Options/BooleanCheckBoxOption.h"
#include "CommonFramework/Options/SimpleIntegerOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "CommonFramework/OCR/OCR_LanguageOptionOCR.h"
#include "NintendoSwitch/Options/TimeExpressionOption.h"
#include "NintendoSwitch/Options/GoHomeWhenDoneOption.h"
#include "NintendoSwitch/Framework/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonBDSP/Options/PokemonBDSP_ShortcutDirection.h"
#include "PokemonBDSP/Options/PokemonBDSP_EggHatchFilter.h"
#include "PokemonBDSP_EggAutonomousState.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{


class EggAutonomous_Descriptor : public RunnableSwitchProgramDescriptor{
public:
    EggAutonomous_Descriptor();
};


class EggAutonomous : public SingleSwitchProgramInstance{
public:
    EggAutonomous(const EggAutonomous_Descriptor& descriptor);

    virtual std::unique_ptr<StatsTracker> make_stats() const override;
    virtual void program(SingleSwitchProgramEnvironment& env) override;

private:
    bool run_batch(
        SingleSwitchProgramEnvironment& env,
        EggAutonomousState& saved_state,
        EggAutonomousState& current_state
    );

private:
    GoHomeWhenDoneOption GO_HOME_WHEN_DONE;

    OCR::LanguageOCR LANGUAGE;

    ShortcutDirection SHORTCUT;
    SimpleIntegerOption<uint8_t> MAX_KEEPERS;
    TimeExpressionOption<uint16_t> TRAVEL_TIME_PER_FETCH;

    EnumDropdownOption AUTO_SAVING;

    EggHatchFilterOption FILTERS;

    EventNotificationOption NOTIFICATION_STATUS_UPDATE;
    EventNotificationOption NOTIFICATION_NONSHINY_KEEP;
    EventNotificationOption NOTIFICATION_SHINY;
    EventNotificationOption NOTIFICATION_PROGRAM_FINISH;
    EventNotificationsOption NOTIFICATIONS;

    SectionDividerOption m_advanced_options;
    TimeExpressionOption<uint16_t> SCROLL_TO_READ_DELAY;
};



}
}
}
#endif
