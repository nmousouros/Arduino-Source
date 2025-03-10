/*  Rolling Auto-Host
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Cpp/PrettyPrint.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Device.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Routines.h"
#include "NintendoSwitch/NintendoSwitch_Settings.h"
#include "NintendoSwitch/FixedInterval.h"
#include "PokemonSwSh/PokemonSwSh_Settings.h"
#include "PokemonSwSh/Commands/PokemonSwSh_Commands_GameEntry.h"
#include "PokemonSwSh/Commands/PokemonSwSh_Commands_DateSpam.h"
#include "PokemonSwSh/Programs/PokemonSwSh_StartGame.h"
#include "PokemonSwSh_DenTools.h"
#include "PokemonSwSh_AutoHostStats.h"
#include "PokemonSwSh_AutoHost.h"
#include "PokemonSwSh_AutoHost-Rolling.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{


AutoHostRolling_Descriptor::AutoHostRolling_Descriptor()
    : RunnableSwitchProgramDescriptor(
        "PokemonSwSh:AutoHostRolling",
        STRING_POKEMON + " SwSh", "Auto-Host Rolling",
        "ComputerControl/blob/master/Wiki/Programs/PokemonSwSh/AutoHost-Rolling.md",
        "Roll N days, host, SR and repeat. Also supports hard-locks and soft-locks.",
        FeedbackType::OPTIONAL_,
        PABotBaseLevel::PABOTBASE_12KB
    )
{}



AutoHostRolling::AutoHostRolling(const AutoHostRolling_Descriptor& descriptor)
    : SingleSwitchProgramInstance(descriptor)
    , SKIPS("<b>Day Skips:</b>", 3)
    , BACKUP_SAVE("<b>Load Backup Save:</b><br>For backup save soft-locking method.", false)
    , HOST_ONLINE("<b>Host Online:</b>", true)
    , LOBBY_WAIT_DELAY(
        "<b>Lobby Wait Delay:</b><br>Wait this long before starting raid. Start time is 3 minutes minus this number.",
        "60 * TICKS_PER_SECOND"
    )
    , FRIEND_ACCEPT_USER_SLOT(
        "<b>Friend Request Accept Slot:</b><br>Zero disables friend accepts.",
        0, 0, 8
    )
    , EXTRA_DELAY_BETWEEN_RAIDS(
        "<b>Extra Delay Between Raids:</b><br>May aid in farming.",
        "0 * TICKS_PER_SECOND"
    )
    , MOVE_SLOT(
        "<b>1st Move Select Slot:</b><br>Zero disables 1st move select.",
        0, 0, 4
    )
    , DYNAMAX(
        "<b>1st Move Dynamax:</b><br>Dynamax on first move. (only applies if above option is non-zero)",
        true
    )
    , TROLL_HOSTING(
        "<b>Troll Hosting:</b> (requires 1st move select)<br>0 disables the troll hosting option, 1 attacks the first ally, 2 attacks the second one, 3 attacks the third one. Dynamaxing will disable this option.",
        0, 0, 3
    )
    , ALTERNATE_GAMES(
        "<b>Alternate Games:</b><br>Alternate hosting between 1st and 2nd games. Host from both Sword and Shield.",
        false
    )
    , HOSTING_NOTIFICATIONS("Live-Hosting Announcements", false)
    , NOTIFICATIONS({
        &HOSTING_NOTIFICATIONS.NOTIFICATION,
        &NOTIFICATION_ERROR_FATAL,
    })
    , m_internet_settings(
        "<font size=4><b>Internet Settings:</b> Increase these if your internet is slow.</font>"
    )
    , CONNECT_TO_INTERNET_DELAY(
        "<b>Connect to Internet Delay:</b><br>Time from \"Connect to Internet\" to when you're ready to enter den.",
        "20 * TICKS_PER_SECOND"
    )
    , ENTER_ONLINE_DEN_DELAY(
        "<b>Enter Online Den Delay:</b><br>\"Communicating\" when entering den while online.",
        "8 * TICKS_PER_SECOND"
    )
    , OPEN_ONLINE_DEN_LOBBY_DELAY(
        "<b>Open Online Den Delay:</b><br>Delay from \"Invite Others\" to when the clock starts ticking.",
        "8 * TICKS_PER_SECOND"
    )
    , RAID_START_TO_EXIT_DELAY(
        "<b>Raid Start to Exit Delay:</b><br>Time from start raid to reset. (when not selecting move)",
        "15 * TICKS_PER_SECOND"
    )
    , DELAY_TO_SELECT_MOVE(
        "<b>Delay to Select Move:</b><br>This + above = time from start raid to select move.",
        "32 * TICKS_PER_SECOND"
    )
{
    PA_ADD_OPTION(START_IN_GRIP_MENU);
    PA_ADD_OPTION(TOUCH_DATE_INTERVAL);

    PA_ADD_OPTION(RAID_CODE);
    PA_ADD_OPTION(SKIPS);
    PA_ADD_OPTION(BACKUP_SAVE);
    PA_ADD_OPTION(HOST_ONLINE);
    PA_ADD_OPTION(LOBBY_WAIT_DELAY);
    PA_ADD_OPTION(CATCHABILITY);
    PA_ADD_OPTION(FRIEND_ACCEPT_USER_SLOT);
    PA_ADD_OPTION(EXTRA_DELAY_BETWEEN_RAIDS);
    PA_ADD_OPTION(MOVE_SLOT);
    PA_ADD_OPTION(DYNAMAX);
    PA_ADD_OPTION(TROLL_HOSTING);
    PA_ADD_OPTION(ALTERNATE_GAMES);
    PA_ADD_OPTION(HOSTING_NOTIFICATIONS);
    PA_ADD_OPTION(NOTIFICATIONS);

    PA_ADD_OPTION(m_internet_settings);
    PA_ADD_OPTION(CONNECT_TO_INTERNET_DELAY);
    PA_ADD_OPTION(ENTER_ONLINE_DEN_DELAY);
    PA_ADD_OPTION(OPEN_ONLINE_DEN_LOBBY_DELAY);
    PA_ADD_OPTION(RAID_START_TO_EXIT_DELAY);
    PA_ADD_OPTION(DELAY_TO_SELECT_MOVE);
}



std::unique_ptr<StatsTracker> AutoHostRolling::make_stats() const{
    return std::unique_ptr<StatsTracker>(new AutoHostStats());
}



void AutoHostRolling::program(SingleSwitchProgramEnvironment& env){
    uint16_t start_raid_delay = HOST_ONLINE
        ? OPEN_ONLINE_DEN_LOBBY_DELAY
        : GameSettings::instance().OPEN_LOCAL_DEN_LOBBY_DELAY;
    const uint16_t lobby_wait_delay = LOBBY_WAIT_DELAY < start_raid_delay
        ? 0
        : LOBBY_WAIT_DELAY - start_raid_delay;

    if (START_IN_GRIP_MENU){
        grip_menu_connect_go_home(env.console);
    }else{
        pbf_press_button(env.console, BUTTON_B, 5, 5);
        pbf_press_button(env.console, BUTTON_HOME, 10, GameSettings::instance().GAME_TO_HOME_DELAY_FAST);
    }

    if (SKIPS == 0){
        TOUCH_DATE_INTERVAL.touch_now_from_home_if_needed(env.console);
    }
    rollback_date_from_home(env.console, SKIPS);
    resume_game_front_of_den_nowatts(env.console, ConsoleSettings::instance().TOLERATE_SYSTEM_UPDATE_MENU_SLOW);

    for (uint32_t raids = 0;; raids++){
        env.log("Raids Attempted: " + tostr_u_commas(raids));
        env.update_stats();

        run_autohost(
            env, env.console,
            CATCHABILITY, SKIPS,
            &RAID_CODE, lobby_wait_delay,
            HOST_ONLINE, FRIEND_ACCEPT_USER_SLOT,
            MOVE_SLOT, DYNAMAX, TROLL_HOSTING,
            HOSTING_NOTIFICATIONS,
            CONNECT_TO_INTERNET_DELAY,
            ENTER_ONLINE_DEN_DELAY,
            OPEN_ONLINE_DEN_LOBBY_DELAY,
            RAID_START_TO_EXIT_DELAY,
            DELAY_TO_SELECT_MOVE
        );

        //  Exit game.
        ssf_press_button2(env.console, BUTTON_HOME, GameSettings::instance().GAME_TO_HOME_DELAY_SAFE, 10);
        close_game(env.console);

        //  Post-raid delay.
        pbf_wait(env.console, EXTRA_DELAY_BETWEEN_RAIDS);

        //  Touch the date.
        if (SKIPS == 0){
            TOUCH_DATE_INTERVAL.touch_now_from_home_if_needed(env.console);
        }else{
            rollback_date_from_home(env.console, SKIPS);
        }

        start_game_from_home_with_inference(
            env, env.console,
            ConsoleSettings::instance().TOLERATE_SYSTEM_UPDATE_MENU_SLOW,
            ALTERNATE_GAMES ? 2 : 0, 0,
            BACKUP_SAVE
        );
    }
}



}
}
}







