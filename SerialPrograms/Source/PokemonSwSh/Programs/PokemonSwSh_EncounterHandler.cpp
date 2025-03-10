/*  Encounter Handler
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <QtGlobal>
#include "Common/Cpp/Exception.h"
#include "CommonFramework/Inference/VisualInferenceRoutines.h"
#include "CommonFramework/Inference/BlackScreenDetector.h"
#include "NintendoSwitch/Commands/NintendoSwitch_PushButtons.h"
#include "Pokemon/Pokemon_Notification.h"
#include "PokemonSwSh/PokemonSwSh_Settings.h"
#include "PokemonSwSh/Programs/PokemonSwSh_BasicCatcher.h"
#include "PokemonSwSh_EncounterHandler.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{


void take_video(const BotBaseContext& context){
    pbf_wait(context, 5 * TICKS_PER_SECOND);
    pbf_press_button(context, BUTTON_CAPTURE, 2 * TICKS_PER_SECOND, 5 * TICKS_PER_SECOND);
//    context->wait_for_all_requests();
}
void run_away(
    ProgramEnvironment& env,
    ConsoleHandle& console,
    uint16_t exit_battle_time
){
    BlackScreenOverWatcher black_screen_detector;
    run_until(
        env, console,
        [=](const BotBaseContext& context){
            pbf_press_dpad(context, DPAD_UP, 10, 0);
            pbf_mash_button(context, BUTTON_A, TICKS_PER_SECOND);
            if (exit_battle_time > TICKS_PER_SECOND){
                pbf_mash_button(context, BUTTON_B, exit_battle_time - TICKS_PER_SECOND);
            }
        },
        { &black_screen_detector }
    );
}




StandardEncounterHandler::StandardEncounterHandler(
    ProgramEnvironment& env,
    ConsoleHandle& console,
    Language language,
    EncounterBotCommonOptions& settings,
    ShinyHuntTracker& session_stats
)
    : m_env(env)
    , m_console(console)
    , m_language(language)
    , m_settings(settings)
    , m_session_stats(session_stats)
//    , m_notification_sender(settings.notification_level)
{}


void StandardEncounterHandler::update_frequencies(StandardEncounterDetection& encounter){
    const std::set<std::string>* slugs = encounter.candidates();
    if (slugs){
        m_frequencies += *slugs;
        m_env.log(m_frequencies.dump_sorted_map("Encounter Stats:\n"));
    }
}
void StandardEncounterHandler::run_away_and_update_stats(
    StandardEncounterDetection& encounter,
    uint16_t exit_battle_time,
    const ShinyDetectionResult& result
){
    //  Initiate the run-away.
    pbf_press_dpad(m_console, DPAD_UP, 10, 0);
    pbf_mash_button(m_console, BUTTON_A, 60);
    m_console.botbase().wait_for_all_requests();

    update_frequencies(encounter);

    const std::set<std::string>* candidates_ptr = encounter.candidates();
    const std::set<std::string>& candidates = candidates_ptr
        ? *candidates_ptr
        : std::set<std::string>();
    send_encounter_notification(
        m_console,
        m_settings.NOTIFICATION_NONSHINY,
        m_settings.NOTIFICATION_SHINY,
        m_env.program_info(),
        candidates_ptr, is_shiny(result.shiny_type),
        {{candidates, result.shiny_type}},
        result.best_screenshot,
        &m_session_stats,
        &m_frequencies
    );

    BlackScreenOverWatcher black_screen_detector;
    int ret = run_until(
        m_env, m_console,
        [=](const BotBaseContext& context){
            pbf_mash_button(context, BUTTON_A, TICKS_PER_SECOND);
            if (exit_battle_time > TICKS_PER_SECOND){
                pbf_mash_button(context, BUTTON_B, exit_battle_time - TICKS_PER_SECOND);
            }
        },
        { &black_screen_detector }
    );
    if (ret < 0){
        m_console.log("Timed out waiting for end of battle. Are you stuck in the battle?", COLOR_RED);
    }
}


bool StandardEncounterHandler::handle_standard_encounter(const ShinyDetectionResult& result){
    if (result.shiny_type == ShinyType::UNKNOWN){
        m_console.log("Unable to determine result of battle.", COLOR_RED);
        m_session_stats.add_error();
        m_consecutive_failures++;
        if (m_consecutive_failures >= 3){
            PA_THROW_StringException("3 consecutive failed encounter detections.");
        }
        return false;
    }
    m_consecutive_failures = 0;

    m_session_stats += result.shiny_type;
    m_env.update_stats();

    if (result.shiny_type == ShinyType::UNKNOWN){
        pbf_mash_button(m_console, BUTTON_B, TICKS_PER_SECOND);
        return false;
    }

    StandardEncounterDetection encounter(
        m_env, m_console,
        m_language,
        m_settings.FILTER,
        result.shiny_type
    );

    update_frequencies(encounter);

    const std::set<std::string>* candidates_ptr = encounter.candidates();
    const std::set<std::string>& candidates = candidates_ptr
        ? *candidates_ptr
        : std::set<std::string>();
    send_encounter_notification(
        m_console,
        m_settings.NOTIFICATION_NONSHINY,
        m_settings.NOTIFICATION_SHINY,
        m_env.program_info(),
        candidates_ptr, is_shiny(result.shiny_type),
        {{candidates, result.shiny_type}},
        result.best_screenshot,
        &m_session_stats,
        &m_frequencies
    );

    if (m_settings.VIDEO_ON_SHINY && encounter.is_shiny()){
        take_video(m_console);
    }

    return encounter.get_action().first == EncounterAction::StopProgram;
}
bool StandardEncounterHandler::handle_standard_encounter_end_battle(
    const ShinyDetectionResult& result,
    uint16_t exit_battle_time
){
    if (result.shiny_type == ShinyType::UNKNOWN){
        m_console.log("Unable to determine result of battle.", COLOR_RED);
        m_session_stats.add_error();
        m_consecutive_failures++;
        if (m_consecutive_failures >= 3){
            PA_THROW_StringException("3 consecutive failed encounter detections.");
        }
        return false;
    }
    m_consecutive_failures = 0;

    m_session_stats += result.shiny_type;
    m_env.update_stats();

    StandardEncounterDetection encounter(
        m_env, m_console,
        m_language,
        m_settings.FILTER,
        result.shiny_type
    );

    if (m_settings.VIDEO_ON_SHINY && encounter.is_shiny()){
        take_video(m_console);
    }

    std::pair<EncounterAction, std::string> action = encounter.get_action();

    //  Fast run-away sequence to save time.
    if (action.first == EncounterAction::RunAway){
        run_away_and_update_stats(encounter, exit_battle_time, result);
        return false;
    }

    update_frequencies(encounter);
    const std::set<std::string>* candidates_ptr = encounter.candidates();
    const std::set<std::string>& candidates = candidates_ptr
        ? *candidates_ptr
        : std::set<std::string>();
    send_encounter_notification(
        m_console,
        m_settings.NOTIFICATION_NONSHINY,
        m_settings.NOTIFICATION_SHINY,
        m_env.program_info(),
        candidates_ptr, is_shiny(result.shiny_type),
        {{candidates, result.shiny_type}},
        result.best_screenshot,
        &m_session_stats,
        &m_frequencies
    );

    switch (action.first){
    case EncounterAction::StopProgram:
        return true;
    case EncounterAction::RunAway:
        return false;
    case EncounterAction::ThrowBalls:{
        CatchResults results = basic_catcher(m_env, m_console, m_language, action.second);
        switch (results.result){
        case CatchResult::POKEMON_CAUGHT:
        case CatchResult::POKEMON_FAINTED:
            break;
        case CatchResult::OWN_FAINTED:
            PA_THROW_StringException("Your " + STRING_POKEMON + " fainted after " + QString::number(results.balls_used) + " balls.");
        case CatchResult::OUT_OF_BALLS:
            PA_THROW_StringException("Unable to find the desired ball after throwing " + QString::number(results.balls_used) + " of them. Did you run out?");
        case CatchResult::CANNOT_THROW_BALL:
            PA_THROW_StringException("Unable to throw ball. Is the " + STRING_POKEMON + " semi-invulnerable?");
        case CatchResult::TIMEOUT:
            PA_THROW_StringException("Program has timed out. Did your lead " + STRING_POKEMON + " faint?");
        }
        send_catch_notification(
            m_console,
            m_settings.NOTIFICATION_CATCH_SUCCESS,
            m_settings.NOTIFICATION_CATCH_FAILED,
            m_env.program_info(),
            encounter.candidates(),
            action.second,
            results.balls_used,
            results.result == CatchResult::POKEMON_CAUGHT
        );
        return false;
    }
    case EncounterAction::ThrowBallsAndSave:{
        CatchResults results = basic_catcher(m_env, m_console, m_language, action.second);
        switch (results.result){
        case CatchResult::POKEMON_CAUGHT:
            pbf_mash_button(m_console, BUTTON_B, 2 * TICKS_PER_SECOND);
            pbf_press_button(m_console, BUTTON_X, 20, GameSettings::instance().OVERWORLD_TO_MENU_DELAY); //  Save game.
            pbf_press_button(m_console, BUTTON_R, 20, 150);
            pbf_press_button(m_console, BUTTON_A, 10, 500);
            break;
        case CatchResult::POKEMON_FAINTED:
            pbf_mash_button(m_console, BUTTON_B, 2 * TICKS_PER_SECOND);
            break;
        case CatchResult::OWN_FAINTED:
            PA_THROW_StringException("Your " + STRING_POKEMON + " fainted after " + QString::number(results.balls_used) + " balls.");
        case CatchResult::OUT_OF_BALLS:
            PA_THROW_StringException("Unable to find the desired ball after throwing " + QString::number(results.balls_used) + " of them. Did you run out?");
        case CatchResult::CANNOT_THROW_BALL:
            PA_THROW_StringException("Unable to throw ball. Is the " + STRING_POKEMON + " semi-invulnerable?");
        case CatchResult::TIMEOUT:
            PA_THROW_StringException("Program has timed out. Did your lead " + STRING_POKEMON + " faint?");
        }
        send_catch_notification(
            m_console,
            m_settings.NOTIFICATION_CATCH_SUCCESS,
            m_settings.NOTIFICATION_CATCH_FAILED,
            m_env.program_info(),
            encounter.candidates(),
            action.second,
            results.balls_used,
            results.result == CatchResult::POKEMON_CAUGHT
        );
        return false;
    }
    }

    return false;
}



}
}
}
