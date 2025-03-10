/*  Mass Release
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "CommonFramework/Tools/StatsTracking.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "NintendoSwitch/Commands/NintendoSwitch_PushButtons.h"
#include "PokemonBDSP/PokemonBDSP_Settings.h"
#include "PokemonBDSP/Programs/PokemonBDSP_BoxRelease.h"
#include "PokemonBDSP_MassRelease.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{


MassRelease_Descriptor::MassRelease_Descriptor()
    : RunnableSwitchProgramDescriptor(
        "PokemonBDSP:MassRelease",
        STRING_POKEMON + " BDSP", "Mass Release",
        "ComputerControl/blob/master/Wiki/Programs/PokemonBDSP/MassRelease.md",
        "Mass release boxes of " + STRING_POKEMON + ".",
        FeedbackType::NONE,
        PABotBaseLevel::PABOTBASE_12KB
    )
{}

MassRelease::MassRelease(const MassRelease_Descriptor& descriptor)
    : SingleSwitchProgramInstance(descriptor)
    , GO_HOME_WHEN_DONE(false)
    , BOXES_TO_RELEASE(
        "<b>Number of Boxes to Release:</b>",
        2, 0, 40
    )
    , NOTIFICATION_PROGRAM_FINISH("Program Finished", true, true)
    , NOTIFICATIONS({
        &NOTIFICATION_PROGRAM_FINISH,
        &NOTIFICATION_ERROR_FATAL,
    })
{
    PA_ADD_OPTION(GO_HOME_WHEN_DONE);
    PA_ADD_OPTION(BOXES_TO_RELEASE);
    PA_ADD_OPTION(NOTIFICATIONS);
}


struct MassRelease::Stats : public StatsTracker{
    Stats()
        : m_boxes_released(m_stats["Boxes Released"])
    {
        m_display_order.emplace_back("Boxes Released");
    }
    std::atomic<uint64_t>& m_boxes_released;
};
std::unique_ptr<StatsTracker> MassRelease::make_stats() const{
    return std::unique_ptr<StatsTracker>(new Stats());
}





void MassRelease::program(SingleSwitchProgramEnvironment& env){
    Stats& stats = env.stats<Stats>();
    env.update_stats();

    //  Connect the controller.
    pbf_press_button(env.console, BUTTON_LCLICK, 5, 5);

    uint16_t box_scroll_delay = GameSettings::instance().BOX_SCROLL_DELAY_0;
    uint16_t box_change_delay = GameSettings::instance().BOX_CHANGE_DELAY_0;

    if (BOXES_TO_RELEASE > 0){
        env.update_stats();
        release_box(env.console, box_scroll_delay);
        stats.m_boxes_released++;
        for (uint8_t box = 1; box < BOXES_TO_RELEASE; box++){
            env.update_stats();
            pbf_press_dpad(env.console, DPAD_DOWN, 20, box_scroll_delay);
            pbf_press_dpad(env.console, DPAD_DOWN, 20, box_scroll_delay);
            pbf_press_dpad(env.console, DPAD_DOWN, 20, box_scroll_delay);
            pbf_press_dpad(env.console, DPAD_RIGHT, 20, box_scroll_delay);
            pbf_press_dpad(env.console, DPAD_RIGHT, 20, box_scroll_delay);
            pbf_wait(env.console, 50);
            pbf_press_button(env.console, BUTTON_R, 20, box_change_delay);
            release_box(env.console, box_scroll_delay);
            stats.m_boxes_released++;
        }
    }

    env.update_stats();
    send_program_finished_notification(
        env.logger(), NOTIFICATION_PROGRAM_FINISH,
        env.program_info(),
        "",
        stats.to_str()
    );
    GO_HOME_WHEN_DONE.run_end_of_program(env.console);
}





}
}
}
