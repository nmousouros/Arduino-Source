/*  Shiny Hunt Autonomous - Overworld
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <cmath>
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "CommonFramework/Inference/VisualInferenceRoutines.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Device.h"
#include "NintendoSwitch/Commands/NintendoSwitch_PushButtons.h"
#include "NintendoSwitch/NintendoSwitch_Settings.h"
#include "PokemonSwSh/PokemonSwSh_Settings.h"
#include "PokemonSwSh/Commands/PokemonSwSh_Commands_GameEntry.h"
#include "PokemonSwSh/Commands/PokemonSwSh_Commands_DateSpam.h"
#include "PokemonSwSh/ShinyHuntTracker.h"
#include "PokemonSwSh/Inference/PokemonSwSh_MarkFinder.h"
#include "PokemonSwSh/Inference/Battles/PokemonSwSh_StartBattleDetector.h"
#include "PokemonSwSh/Inference/Battles/PokemonSwSh_BattleMenuDetector.h"
#include "PokemonSwSh/Programs/PokemonSwSh_StartGame.h"
#include "PokemonSwSh/Programs/PokemonSwSh_EncounterHandler.h"
#include "PokemonSwSh_OverworldMovement.h"
#include "PokemonSwSh_OverworldTrajectory.h"
#include "PokemonSwSh_OverworldTrigger.h"
#include "PokemonSwSh_ShinyHuntAutonomous-Overworld.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{


ShinyHuntAutonomousOverworld_Descriptor::ShinyHuntAutonomousOverworld_Descriptor()
    : RunnableSwitchProgramDescriptor(
        "PokemonSwSh:ShinyHuntAutonomousOverworld",
        STRING_POKEMON + " SwSh", "Shiny Hunt Autonomous - Overworld",
        "ComputerControl/blob/master/Wiki/Programs/PokemonSwSh/ShinyHuntAutonomous-Overworld.md",
        "Automatically shiny hunt overworld " + STRING_POKEMON + " with video feedback.",
        FeedbackType::REQUIRED,
        PABotBaseLevel::PABOTBASE_12KB
    )
{}



ShinyHuntAutonomousOverworld::ShinyHuntAutonomousOverworld(const ShinyHuntAutonomousOverworld_Descriptor& descriptor)
    : SingleSwitchProgramInstance(descriptor)
    , GO_HOME_WHEN_DONE(false)
    , TIME_ROLLBACK_HOURS(
        "<b>Time Rollback (in hours):</b><br>Periodically roll back the time to keep the weather the same. If set to zero, this feature is disabled.",
        1, 0, 11
    )
    , MARK_OFFSET(
        "<b>Mark Offset:</b><br>Aim this far below the bottom of the exclamation/question mark. 1.0 is the height of the mark. "
        "Increase this value when the " + STRING_POKEMON + " are large.",
        0.5, 0, 20
    )
    , MARK_PRIORITY(
        "<b>Mark Priority:</b><br>Favor exclamation marks or question marks?",
        MARK_PRIORITY_STRINGS, 1
    )
    , TRIGGER_METHOD(
        "<b>Trigger Method:</b><br>How to trigger an overworld reaction mark?",
        {
            "Whistle Only",
            "Whistle 3 times, then circle once.",
            "Circle 3 times, then whistle 3 times.",
            "Circle Only",
            "Horizontal Line Only",
            "Whistle 3 times, then do horizontal line once.",
            "Do horizontal line 3 times, then whistle 3 times.",
            "Vertical Line Only",
            "Whistle 3 times, then do vertical line once.",
            "Do vertical line 3 times, then whistle 3 times.",
        }, 1
    )
    , MAX_MOVE_DURATION(
        "<b>Maximum Move Duration:</b><br>Do not move in the same direction for more than this long."
        " If you set this too high, you may wander too far from the grassy area.",
        "200"
    )
    , MAX_TARGET_ALPHA(
        "<b>Max Target Alpha:</b><br>Ignore all targets with alpha larger than this. Set to zero to ignore all marks.",
        70000, 0
    )
    , ENCOUNTER_BOT_OPTIONS(true, true)
    , NOTIFICATION_PROGRAM_FINISH("Program Finished", true, true)
    , NOTIFICATIONS({
        &ENCOUNTER_BOT_OPTIONS.NOTIFICATION_NONSHINY,
        &ENCOUNTER_BOT_OPTIONS.NOTIFICATION_SHINY,
        &ENCOUNTER_BOT_OPTIONS.NOTIFICATION_CATCH_SUCCESS,
        &ENCOUNTER_BOT_OPTIONS.NOTIFICATION_CATCH_FAILED,
        &NOTIFICATION_PROGRAM_FINISH,
        &NOTIFICATION_ERROR_FATAL,
    })
    , m_advanced_options(
        "<font size=4><b>Advanced Options:</b> You should not need to touch anything below here.</font>"
    )
    , WATCHDOG_TIMER(
        "<b>Watchdog Timer:</b><br>Reset the game if you go this long without any encounters.",
        "60 * TICKS_PER_SECOND"
    )
    , EXIT_BATTLE_TIMEOUT(
        "<b>Exit Battle Timeout:</b><br>After running, wait this long to return to overworld.",
        "10 * TICKS_PER_SECOND"
    )
    , TARGET_CIRCLING(
        "<b>Target Circling:</b><br>After moving towards a " + STRING_POKEMON + ", make a circle."
        " This increases the chance of encountering the " + STRING_POKEMON + " if it has moved or if the trajectory missed.",
        true
    )
{
    PA_ADD_OPTION(START_IN_GRIP_MENU);
    PA_ADD_OPTION(GO_HOME_WHEN_DONE);
    PA_ADD_OPTION(TIME_ROLLBACK_HOURS);

    PA_ADD_OPTION(LANGUAGE);
    PA_ADD_OPTION(MARK_OFFSET);
    PA_ADD_OPTION(MARK_PRIORITY);
    PA_ADD_OPTION(TRIGGER_METHOD);
    PA_ADD_OPTION(MAX_MOVE_DURATION);
    PA_ADD_OPTION(MAX_TARGET_ALPHA);

    PA_ADD_OPTION(ENCOUNTER_BOT_OPTIONS);
    PA_ADD_OPTION(NOTIFICATIONS);

    PA_ADD_STATIC(m_advanced_options);
    PA_ADD_OPTION(WATCHDOG_TIMER);
    PA_ADD_OPTION(EXIT_BATTLE_TIMEOUT);
    PA_ADD_OPTION(TARGET_CIRCLING);
}



struct ShinyHuntAutonomousOverworld::Stats : public ShinyHuntTracker{
    Stats()
        : ShinyHuntTracker(true)
        , m_resets(m_stats["Resets"])
    {
        m_display_order.insert(m_display_order.begin() + 2, Stat("Resets"));
        m_aliases["Unexpected Battles"] = "Errors";
    }
    std::atomic<uint64_t>& m_resets;
};
std::unique_ptr<StatsTracker> ShinyHuntAutonomousOverworld::make_stats() const{
    return std::unique_ptr<StatsTracker>(new Stats());
}






bool ShinyHuntAutonomousOverworld::find_encounter(
    SingleSwitchProgramEnvironment& env,
    Stats& stats,
    std::chrono::system_clock::time_point expiration
) const{
    InferenceBoxScope self(
        env.console,
        OverworldTargetTracker::OVERWORLD_CENTER_X - 0.02,
        OverworldTargetTracker::OVERWORLD_CENTER_Y - 0.05,
        0.04, 0.1, COLOR_CYAN
    );

    OverworldTargetTracker target_tracker(
        env.console, env.console,
        std::chrono::milliseconds(1000),
        MARK_OFFSET,
        (MarkPriority)(size_t)MARK_PRIORITY,
        MAX_TARGET_ALPHA
    );

    std::unique_ptr<OverworldTrigger> trigger;
    switch ((size_t)TRIGGER_METHOD){
    case 0:
        trigger.reset(new OverworldTrigger_Whistle(target_tracker));
        break;
    case 1:
        trigger.reset(new OverworldTrigger_WhistleCircle(target_tracker, true, 3, 1));
        break;
    case 2:
        trigger.reset(new OverworldTrigger_WhistleCircle(target_tracker, false, 3, 3));
        break;
    case 3:
        trigger.reset(new OverworldTrigger_WhistleCircle(target_tracker, false, 0, 1));
        break;
    case 4:
        trigger.reset(new OverworldTrigger_WhistleHorizontal(target_tracker, false, 0, 1));
        break;
    case 5:
        trigger.reset(new OverworldTrigger_WhistleHorizontal(target_tracker, true, 3, 1));
        break;
    case 6:
        trigger.reset(new OverworldTrigger_WhistleHorizontal(target_tracker, false, 3, 3));
        break;
    case 7:
        trigger.reset(new OverworldTrigger_WhistleVertical(target_tracker, false, 0, 1));
        break;
    case 8:
        trigger.reset(new OverworldTrigger_WhistleVertical(target_tracker, true, 3, 1));
        break;
    case 9:
        trigger.reset(new OverworldTrigger_WhistleVertical(target_tracker, false, 3, 3));
        break;
    }

    while (true){
        //  Time expired.
        if (std::chrono::system_clock::now() > expiration){
            return false;
        }

        target_tracker.clear_detections();

        //  Run trigger.
        {
            StandardBattleMenuWatcher battle_menu_detector(false);
            StartBattleWatcher start_battle_detector;

            int result = run_until(
                env, env.console,
                [&](const BotBaseContext& context){
                    trigger->run(context);
                },
                {
                    &battle_menu_detector,
                    &start_battle_detector,
                    &target_tracker,
                }
            );

            switch (result){
            case 0:
                env.console.log("Unexpected Battle.", COLOR_RED);
                return false;
            case 1:
                env.console.log("Battle started!");
                return true;
            }
        }

        std::pair<double, OverworldTarget> target = target_tracker.best_target();
        target_tracker.clear_detections();

//        env.log("target: " + std::to_string(target.first));

        if (target.first < 0){
            env.log("No targets found.", COLOR_ORANGE);
            continue;
        }
        if (target.first > MAX_TARGET_ALPHA){
            env.log(
                QString("Target too Weak: ") +
                (target.second.mark == OverworldMark::EXCLAMATION_MARK ? "Exclamation" : "Question") +
                " at [" +
                QString::number(target.second.delta_x) + " , " +
                QString::number(-target.second.delta_y) + "], alpha = " +
                QString::number(target.first),
                COLOR_ORANGE
            );
            continue;
        }

        if (charge_at_target(env, env.console, target)){
            return true;
        }
    }
}


bool ShinyHuntAutonomousOverworld::charge_at_target(
    ProgramEnvironment& env, ConsoleHandle& console,
    const std::pair<double, OverworldTarget>& target
) const{
    InferenceBoxScope target_box(console, target.second.box, COLOR_YELLOW);
    env.log(
        QString("Best Target: ") +
        (target.second.mark == OverworldMark::EXCLAMATION_MARK ? "Exclamation" : "Question") +
        " at [" +
        QString::number(target.second.delta_x) + " , " +
        QString::number(-target.second.delta_y) + "], alpha = " +
        QString::number(target.first),
        COLOR_PURPLE
    );

    const Trajectory& trajectory = target.second.trajectory;
    double angle = std::atan2(
        (double)trajectory.joystick_y - 128,
        (double)trajectory.joystick_x - 128
    ) * 57.295779513082320877;
    env.log(
        "Trajectory: Distance = " + QString::number(trajectory.distance_in_ticks) +
        ", Direction = " + QString::number(-angle) + " degrees"
    );

    int duration = trajectory.distance_in_ticks + 16;
    if (duration > (int)MAX_MOVE_DURATION){
        duration = MAX_MOVE_DURATION;
    }


    StandardBattleMenuWatcher battle_menu_detector(false);
    StartBattleWatcher start_battle_detector;
    OverworldTargetTracker target_tracker(
        console, console,
        std::chrono::milliseconds(1000),
        MARK_OFFSET,
        (MarkPriority)(size_t)MARK_PRIORITY,
        MAX_TARGET_ALPHA
    );

    int result = run_until(
        env, console,
        [&](const BotBaseContext& context){
            //  Move to target.
            pbf_move_left_joystick(
                context,
                trajectory.joystick_x,
                trajectory.joystick_y,
                (uint16_t)duration, 0
            );

            //  Circle Maneuver
            if (TARGET_CIRCLING){
                if (trajectory.joystick_y < 64 &&
                    64 <= trajectory.joystick_x && trajectory.joystick_x <= 192
                ){
                    move_in_circle_up(context, trajectory.joystick_x > 128);
                }else{
                    move_in_circle_down(context, trajectory.joystick_x <= 128);
                }
            }
        },
        {
            &battle_menu_detector,
            &start_battle_detector,
            &target_tracker,
        }
    );

    switch (result){
    case 0:
        console.log("Unexpected Battle.", COLOR_RED);
        return true;
    case 1:
        console.log("Battle started!");
        return true;
    default:
        return false;
    }
}






void ShinyHuntAutonomousOverworld::program(SingleSwitchProgramEnvironment& env){
    srand(time(nullptr));

    if (START_IN_GRIP_MENU){
        grip_menu_connect_go_home(env.console);
        resume_game_back_out(env.console, ConsoleSettings::instance().TOLERATE_SYSTEM_UPDATE_MENU_FAST, 200);
    }else{
        pbf_press_button(env.console, BUTTON_B, 5, 5);
    }
    pbf_move_right_joystick(env.console, 128, 255, TICKS_PER_SECOND, 0);

    const std::chrono::milliseconds TIMEOUT((uint64_t)WATCHDOG_TIMER * 1000 / TICKS_PER_SECOND);
    const uint32_t PERIOD = (uint32_t)TIME_ROLLBACK_HOURS * 3600 * TICKS_PER_SECOND;
    uint32_t last_touch = system_clock(env.console);

    Stats& stats = env.stats<Stats>();
    env.update_stats();

    StandardEncounterHandler handler(
        env, env.console,
        LANGUAGE,
        ENCOUNTER_BOT_OPTIONS,
        stats
    );

    //  Encounter Loop
    auto last = std::chrono::system_clock::now();
    while (true){
        //  Touch the date.
        if (TIME_ROLLBACK_HOURS > 0 && system_clock(env.console) - last_touch >= PERIOD){
            pbf_press_button(env.console, BUTTON_HOME, 10, GameSettings::instance().GAME_TO_HOME_DELAY_SAFE);
            rollback_hours_from_home(env.console, TIME_ROLLBACK_HOURS, ConsoleSettings::instance().SETTINGS_TO_HOME_DELAY);
            resume_game_no_interact(env.console, ConsoleSettings::instance().TOLERATE_SYSTEM_UPDATE_MENU_FAST);
            last_touch += PERIOD;
        }

//        cout << "TOLERATE_SYSTEM_UPDATE_MENU_FAST = " << TOLERATE_SYSTEM_UPDATE_MENU_FAST << endl;

        auto now = std::chrono::system_clock::now();
        if (now - last > TIMEOUT){
            pbf_press_button(env.console, BUTTON_HOME, 10, GameSettings::instance().GAME_TO_HOME_DELAY_SAFE);
            reset_game_from_home_with_inference(
                env, env.console,
                ConsoleSettings::instance().TOLERATE_SYSTEM_UPDATE_MENU_FAST
            );
            stats.m_resets++;
            last = std::chrono::system_clock::now();
            continue;
        }

        env.console.botbase().wait_for_all_requests();

        bool battle = find_encounter(env, stats, last + TIMEOUT);
        if (!battle){
            continue;
        }

        //  Detect shiny.
        ShinyDetectionResult result = detect_shiny_battle(
            env.console,
            env, env.console, env.console,
            SHINY_BATTLE_REGULAR,
            std::chrono::seconds(30)
        );
//        shininess = ShinyDetection::SQUARE_SHINY;

        bool stop = handler.handle_standard_encounter_end_battle(result, EXIT_BATTLE_TIMEOUT);
        if (stop){
            break;
        }

        last = std::chrono::system_clock::now();
    }

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

