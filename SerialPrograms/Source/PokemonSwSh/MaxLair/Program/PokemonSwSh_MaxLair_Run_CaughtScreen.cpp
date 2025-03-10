/*  Max Lair Run Caught Screen
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Cpp/Exception.h"
#include "CommonFramework/Tools/ErrorDumper.h"
#include "CommonFramework/Tools/InterruptableCommands.h"
#include "CommonFramework/Inference/VisualInferenceRoutines.h"
#include "NintendoSwitch/NintendoSwitch_Settings.h"
#include "NintendoSwitch/Commands/NintendoSwitch_PushButtons.h"
#include "PokemonSwSh/PokemonSwSh_Settings.h"
#include "PokemonSwSh/Inference/PokemonSwSh_SummaryShinySymbolDetector.h"
#include "PokemonSwSh/Programs/PokemonSwSh_StartGame.h"
#include "PokemonSwSh/MaxLair/Inference/PokemonSwSh_MaxLair_Detect_EndBattle.h"
#include "PokemonSwSh/MaxLair/Inference/PokemonSwSh_MaxLair_Detect_Entrance.h"
#include "PokemonSwSh/MaxLair/Framework/PokemonSwSh_MaxLair_Notifications.h"
#include "PokemonSwSh/MaxLair/Framework/PokemonSwSh_MaxLair_CatchScreenTracker.h"
#include "PokemonSwSh_MaxLair_Run_CaughtScreen.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{
namespace MaxLairInternal{


StateMachineAction mash_A_to_entrance(
    AdventureRuntime& runtime,
    ProgramEnvironment& env,
    ConsoleHandle& console,
    const QImage& entrance
){
    EntranceDetector entrance_detector(entrance);

    int result = run_until(
        env, console,
        [&](const BotBaseContext& context){
            pbf_mash_button(context, BUTTON_A, 60 * TICKS_PER_SECOND);
        },
        { &entrance_detector },
        INFERENCE_RATE
    );

    if (result < 0){
        console.log("Failed to detect entrance.", COLOR_RED);
//        PA_THROW_StringException("Failed to detect entrance.");
        runtime.session_stats.add_error();
        dump_image(console, MODULE_NAME, "ResetRecovery", console.video().snapshot());
        return StateMachineAction::RESET_RECOVER;
    }
    return StateMachineAction::KEEP_GOING;
}


void synchronize_caught_screen(
    ProgramEnvironment& env,
    ConsoleHandle& console,
    GlobalStateTracker& state_tracker
){
    console.botbase().wait_for_all_requests();
    state_tracker.synchronize(env, console, console.index(), std::chrono::seconds(60));
}


StateMachineAction run_caught_screen(
    AdventureRuntime& runtime,
    ProgramEnvironment& env,
    ConsoleHandle& console,
    GlobalStateTracker& state_tracker,
    const EndBattleDecider& decider,
    const QImage& entrance
){
    size_t console_index = console.index();
    bool is_host = console_index == runtime.host_index;

    pbf_wait(console, TICKS_PER_SECOND);
    console.botbase().wait_for_all_requests();

    CaughtPokemonScreen tracker(env, console);
    runtime.session_stats.add_run(tracker.total());
    if (is_host){
        runtime.path_stats.add_run(tracker.total() >= 4);
//        cout << runtime.path_stats.to_str() << endl;
    }

    //  Scroll over everything. This checks them for shinies.
    tracker.enter_summary();
    for (size_t c = 0; c < tracker.total(); c++){
        tracker.scroll_to(c);
    }

    //  Get all the shinies.
    bool boss_is_shiny = false;
    std::vector<size_t> shinies;
    for (size_t c = 0; c < tracker.total(); c++){
        if (!tracker[c].shiny){
            continue;
        }
        shinies.emplace_back(c);
        runtime.session_stats.add_shiny();
        if (c == 3){
            boss_is_shiny = true;
            runtime.session_stats.add_shiny_legendary();
        }
    }

    //  If anything is shiny, take a video.
    if (!shinies.empty()){
        tracker.scroll_to(shinies.back());
        tracker.leave_summary();
        env.wait_for(std::chrono::seconds(1));
        pbf_press_button(console, BUTTON_CAPTURE, 2 * TICKS_PER_SECOND, 5 * TICKS_PER_SECOND);
        console.botbase().wait_for_all_requests();
    }

    //  Screencap all the shinies and send notifications.
    for (size_t index : shinies){
        tracker.scroll_to(index);
        tracker.leave_summary();
        QImage screen = console.video().snapshot();

        std::lock_guard<std::mutex> lg(env.lock());
        send_shiny_notification(
            console, runtime.notification_shiny,
            env.program_info(),
            console_index, shinies.size(),
            nullptr,
            runtime.path_stats,
            runtime.session_stats,
            screen
        );
    }


    const std::string& boss = state_tracker[console_index].boss;
    CaughtScreenAction action =
        decider.end_adventure_action(
            console_index, boss,
            runtime.path_stats,
            !shinies.empty(), boss_is_shiny
        );

    switch (action){
    case CaughtScreenAction::STOP_PROGRAM:
        console.log("Stopping program...", COLOR_PURPLE);
        synchronize_caught_screen(env, console, state_tracker);
        return StateMachineAction::STOP_PROGRAM;

    case CaughtScreenAction::TAKE_NON_BOSS_SHINY_AND_CONTINUE:
        if (is_host){
            runtime.path_stats.clear();
        }
        if (shinies.empty() || shinies[0] == 3){
            console.log("Quitting back to entrance.", COLOR_PURPLE);
            tracker.leave_summary();
            synchronize_caught_screen(env, console, state_tracker);
            pbf_press_dpad(console, DPAD_DOWN, 10, 50);
            pbf_press_button(console, BUTTON_B, 10, TICKS_PER_SECOND);
            return mash_A_to_entrance(runtime, env, console, entrance);
        }else{
            console.log("Taking non-shiny boss and returning to entrance...", COLOR_BLUE);
            tracker.scroll_to(shinies[0]);
            tracker.enter_summary();    //  Enter summary to verify you're on the right mon.
            tracker.leave_summary();
            synchronize_caught_screen(env, console, state_tracker);
            return mash_A_to_entrance(runtime, env, console, entrance);
        }

    case CaughtScreenAction::RESET:
        console.log("Resetting game...", COLOR_BLUE);
        synchronize_caught_screen(env, console, state_tracker);
        pbf_press_button(console, BUTTON_HOME, 10, GameSettings::instance().GAME_TO_HOME_DELAY_SAFE);
        reset_game_from_home_with_inference(env, console, ConsoleSettings::instance().TOLERATE_SYSTEM_UPDATE_MENU_FAST);
        return StateMachineAction::DONE_WITH_ADVENTURE;
    }

    PA_THROW_StringException("Invalid enum.");
}


}
}
}
}
