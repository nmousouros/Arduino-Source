/*  Walking Pokemon Berry Farmer
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "CommonFramework/Tools/StatsTracking.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "NintendoSwitch/Commands/NintendoSwitch_PushButtons.h"
#include "PokemonBDSP/PokemonBDSP_Settings.h"
#include "PokemonBDSP_AmitySquarePickUpFarmer.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{


AmitySquarePickUpFarmer_Descriptor::AmitySquarePickUpFarmer_Descriptor()
    : RunnableSwitchProgramDescriptor(
        "PokemonBDSP:AmitySquarePickUpFarmer",
        STRING_POKEMON + " BDSP", "Amity Square Pick Up Farmer",
        "ComputerControl/blob/master/Wiki/Programs/PokemonBDSP/AmitySquarePickUpFarmer.md",
        "Automatically fetch berries and stickers from the walking pokemon in Amity Square.",
        FeedbackType::NONE,
        PABotBaseLevel::PABOTBASE_12KB
    )
{}


AmitySquarePickUpFarmer::AmitySquarePickUpFarmer(const AmitySquarePickUpFarmer_Descriptor& descriptor)
    : SingleSwitchProgramInstance(descriptor)
    , GO_HOME_WHEN_DONE(false)
    , MAX_FETCH_ATTEMPTS(
        "<b>Fetch this many times:</b><br>This puts a limit on how many items you can get.",
        100
    )
    , ONE_WAY_MOVING_TIME(
        "<b>One Way walking Time:</b><br>Walk this amount of time in one direction before going back to finish one round of walking.",
        "5 * TICKS_PER_SECOND"
    )
    , ROUNDS_PER_FETCH(
        "<b>Rounds per fetch:</b><br>How many rounds of walking before doing a berry fetch attempt.",
        3
    )
    , WAIT_TIME_FOR_POKEMON(
        "<b>Wait Time for Pokemon:</b><br>Wait this time for pokemon to catch up to you before you ask for a berry.",
        "3 * TICKS_PER_SECOND"
    )
    , NOTIFICATION_STATUS_UPDATE("Status Update", true, false, std::chrono::seconds(3600))
    , NOTIFICATION_PROGRAM_FINISH("Program Finished", true, true)
    , NOTIFICATIONS({
        &NOTIFICATION_STATUS_UPDATE,
        &NOTIFICATION_PROGRAM_FINISH,
        &NOTIFICATION_ERROR_FATAL,
    })
{
    PA_ADD_OPTION(GO_HOME_WHEN_DONE);
    PA_ADD_OPTION(MAX_FETCH_ATTEMPTS);
    PA_ADD_OPTION(ONE_WAY_MOVING_TIME);
    PA_ADD_OPTION(ROUNDS_PER_FETCH);
    PA_ADD_OPTION(WAIT_TIME_FOR_POKEMON);
    PA_ADD_OPTION(NOTIFICATIONS);
}


struct AmitySquarePickUpFarmer::Stats : public StatsTracker{
    Stats()
        : m_attempts(m_stats["Fetch Attempts"])
    {
        m_display_order.emplace_back("Fetch Attempts");
    }
    std::atomic<uint64_t>& m_attempts;
};
std::unique_ptr<StatsTracker> AmitySquarePickUpFarmer::make_stats() const{
    return std::unique_ptr<StatsTracker>(new Stats());
}


void AmitySquarePickUpFarmer::program(SingleSwitchProgramEnvironment& env){
    Stats& stats = env.stats<Stats>();
    env.update_stats();

    //  Connect the controller.
    pbf_move_right_joystick(env.console, 0, 255, 10, 0);

    for (uint16_t c = 0; c < MAX_FETCH_ATTEMPTS; c++) {
        env.update_stats();
        send_program_status_notification(
            env.logger(), NOTIFICATION_STATUS_UPDATE,
            env.program_info(),
            "",
            stats.to_str()
        );

        for (uint16_t i = 0; i < ROUNDS_PER_FETCH; i++) {
            //  Move right
            pbf_move_left_joystick(env.console, 255, 128, ONE_WAY_MOVING_TIME, 0);
            // Move left
            pbf_move_left_joystick(env.console, 0, 128, ONE_WAY_MOVING_TIME, 0);
        }

        // Wait for your pokemon to catch up to you
        pbf_wait(env.console, WAIT_TIME_FOR_POKEMON);

        // Face toward your pokemon.
        pbf_press_dpad(env.console, DPAD_RIGHT, 1, 0);

        // Mash button to talk to pokemon
        pbf_mash_button(env.console, BUTTON_ZL, 500);

        // Mash button to end talking to pokemon
        pbf_mash_button(env.console, BUTTON_B, 500);

        stats.m_attempts++;
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
