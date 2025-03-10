/*  Max Lair Run Start
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Cpp/Exception.h"
#include "CommonFramework/Tools/ErrorDumper.h"
#include "CommonFramework/Tools/InterruptableCommands.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "CommonFramework/Inference/VisualInferenceRoutines.h"
#include "NintendoSwitch/Commands/NintendoSwitch_PushButtons.h"
#include "NintendoSwitch/Commands/NintendoSwitch_DigitEntry.h"
#include "Pokemon/Resources/Pokemon_PokemonNames.h"
#include "PokemonSwSh/Programs/PokemonSwSh_Internet.h"
#include "PokemonSwSh/Programs/Hosting/PokemonSwSh_DenTools.h"
#include "PokemonSwSh/MaxLair/Inference/PokemonSwSh_MaxLair_Detect_PokemonReader.h"
#include "PokemonSwSh/MaxLair/Inference/PokemonSwSh_MaxLair_Detect_Lobby.h"
#include "PokemonSwSh/MaxLair/Inference/PokemonSwSh_MaxLair_Detect_Entrance.h"
#include "PokemonSwSh/MaxLair/Framework/PokemonSwSh_MaxLair_Notifications.h"
#include "PokemonSwSh_MaxLair_Run_EnterLobby.h"
#include "PokemonSwSh_MaxLair_Run_Start.h"
#include "PokemonSwSh_MaxLair_Run_StartSolo.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{
namespace MaxLairInternal{


bool abort_if_error(MultiSwitchProgramEnvironment& env, const std::atomic<size_t>& errors){
    if (errors.load(std::memory_order_acquire)){
        env.run_in_parallel([&](ConsoleHandle& console){
            pbf_press_button(console, BUTTON_B, 10, TICKS_PER_SECOND);
            pbf_press_button(console, BUTTON_A, 10, TICKS_PER_SECOND);
            pbf_mash_button(console, BUTTON_B, 8 * TICKS_PER_SECOND);
        });
        return true;
    }
    return false;
}

bool wait_for_all_join(
    ProgramEnvironment& env, ConsoleHandle& console,
    const QImage& entrance,
    size_t start_players
){
    LobbyJoinedDetector joined_detector(start_players, false);
    EntranceDetector entrance_detector(entrance);
    int result = wait_until(
        env, console,
        std::chrono::seconds(10),
        {
            &joined_detector,
            &entrance_detector,
        },
        INFERENCE_RATE
    );
    switch (result){
    case 0:
        console.log("Detected " + std::to_string(start_players) + " players in lobby!");
        return true;
    case 1:
        console.log("Detected entrance... Did you get disconnected?", COLOR_RED);
//        dump_image(console, MODULE_NAME, "wait_for_all_join", console.video().snapshot());
        return false;
    default:
        console.log("Timed out waiting for everyone to join.", COLOR_RED);
//        dump_image(console, MODULE_NAME, "wait_for_all_join", console.video().snapshot());
        return false;
    }
}

class AllJoinedTracker{
public:
    AllJoinedTracker(
        ProgramEnvironment& env, size_t consoles,
        std::chrono::system_clock::time_point time_limit
    )
        : m_env(env)
        , m_time_limit(time_limit)
        , m_consoles(consoles)
        , m_counter(0)
    {}

    bool report_joined(){
        std::unique_lock<std::mutex> lg(m_env.lock());
        m_counter++;
        if (m_counter >= m_consoles){
            m_env.cv().notify_all();
            return true;
        }
        while (true){
            m_env.cv().wait_until(lg, m_time_limit);
            m_env.check_stopping();
            if (m_counter >= m_consoles){
                return true;
            }
            if (std::chrono::system_clock::now() > m_time_limit){
                return false;
            }
        }
    }

private:
    ProgramEnvironment& m_env;
    std::chrono::system_clock::time_point m_time_limit;
    size_t m_consoles;
    size_t m_counter;
};






bool start_raid_local(
    MultiSwitchProgramEnvironment& env,
    GlobalStateTracker& state_tracker,
    QImage entrance[4],
    ConsoleHandle& host, size_t boss_slot,
    const HostingSettings& settings,
    ConsoleRuntime console_stats[4]
){
    if (env.consoles.size() == 1){
        return start_raid_self_solo(env, host, state_tracker, entrance[0], boss_slot, console_stats[0].ore);
    }

    env.log("Entering lobby...");

    uint8_t code[8];
    bool raid_code = settings.RAID_CODE.get_code(code);

    std::atomic<size_t> errors(0);
    env.run_in_parallel([&](ConsoleHandle& console){
        size_t index = console.index();
        bool is_host = index == host.index();

        entrance[index] = enter_lobby(
            env, console,
            is_host ? boss_slot : 0,
            (HostingMode)(size_t)settings.MODE == HostingMode::HOST_ONLINE,
            console_stats[index].ore
        );
        if (entrance[index].isNull()){
            errors.fetch_add(1);
            return;
        }
    });
    if (errors.load(std::memory_order_acquire) != 0){
        env.run_in_parallel([&](ConsoleHandle& console){
            pbf_mash_button(console, BUTTON_B, 8 * TICKS_PER_SECOND);
        });
        return false;
    }

    env.run_in_parallel([&](ConsoleHandle& console){
        size_t index = console.index();
        GlobalState& state = state_tracker[index];
        bool is_host = index == host.index();

        //  Read boss.
        if (is_host){
            state.boss = read_boss_sprite(console);
        }

        //  Enter code.
        if (raid_code && env.consoles.size() > 1){
            pbf_press_button(console, BUTTON_PLUS, 10, TICKS_PER_SECOND);
            enter_digits(console, 8, code);
            pbf_wait(console, 2 * TICKS_PER_SECOND);
            pbf_press_button(console, BUTTON_A, 10, TICKS_PER_SECOND);
        }
    });

    //  Open lobby.
    env.run_in_parallel([&](ConsoleHandle& console){
        //  Delay to prevent the Switches from forming separate lobbies.
        if (env.consoles.size() > 1 && console.index() != host.index()){
            pbf_wait(console, 3 * TICKS_PER_SECOND);
        }
        pbf_press_button(console, BUTTON_A, 10, TICKS_PER_SECOND);
    });

    auto time_limit = std::chrono::system_clock::now() +
        std::chrono::milliseconds(settings.LOBBY_WAIT_DELAY * 1000 / TICKS_PER_SECOND);

    AllJoinedTracker joined_tracker(env, env.consoles.size(), time_limit);

    //  Wait for all Switches to join.
    env.run_in_parallel([&](ConsoleHandle& console){
        size_t index = console.index();

        //  Wait for a player to show up. This lets you ready up.
        if (!wait_for_a_player(env, console, entrance[index], time_limit)){
            errors.fetch_add(1);
            return;
        }
    });
    if (abort_if_error(env, errors)){
        return false;
    }

    env.run_in_parallel([&](ConsoleHandle& console){
        //  Wait for all consoles to join.
        if (!joined_tracker.report_joined()){
            console.log("Not everyone was able to join.", COLOR_RED);
            errors.fetch_add(1);
            return;
        }
    });
    if (abort_if_error(env, errors)){
        return false;
    }

    env.run_in_parallel([&](ConsoleHandle& console){
        size_t index = console.index();
        if (!wait_for_all_join(env, console, entrance[index], env.consoles.size())){
            console.log("Switches joined into different raids.", COLOR_RED);
            errors.fetch_add(1);
            return;
        }
    });
    if (abort_if_error(env, errors)){
        return false;
    }

    //  Ready up and wait for lobby to be ready.
    env.run_in_parallel([&](ConsoleHandle& console){
        //  Ready up.
        env.wait_for(std::chrono::seconds(1));
        pbf_press_button(console, BUTTON_A, 10, TICKS_PER_SECOND);
        console.botbase().wait_for_all_requests();

        //  Wait
        size_t index = console.index();
        if (!wait_for_lobby_ready(env, console, entrance[index], env.consoles.size(), env.consoles.size(), time_limit)){
            errors.fetch_add(1);
            pbf_mash_button(console, BUTTON_B, 10 * TICKS_PER_SECOND);
            return;
        }
    });
    if (abort_if_error(env, errors)){
        return false;
    }

    env.run_in_parallel([&](ConsoleHandle& console){
        //  Start
        size_t index = console.index();
        if (!start_adventure(env, console, env.consoles.size(), entrance[index])){
            errors.fetch_add(1);
            pbf_mash_button(console, BUTTON_B, 10 * TICKS_PER_SECOND);
            return;
        }
    });
    if (abort_if_error(env, errors)){
        return false;
    }

    return true;
}

bool start_raid_host(
    MultiSwitchProgramEnvironment& env,
    GlobalStateTracker& state_tracker,
    QImage entrance[4],
    ConsoleHandle& host, size_t boss_slot,
    HostingSettings& settings,
    const PathStats& path_stats,
    const StatsTracker& session_stats,
    ConsoleRuntime console_stats[4]
){
    if (env.consoles.size() == 1){
        return start_raid_host_solo(
            env, host,
            state_tracker,
            entrance[0], boss_slot,
            settings,
            path_stats, session_stats,
            console_stats[0].ore
        );
    }

    env.log("Entering lobby...");

    uint8_t code[8];
    bool has_code = settings.RAID_CODE.get_code(code);
    std::string boss;

    std::atomic<size_t> errors(0);
    env.run_in_parallel([&](ConsoleHandle& console){
        size_t index = console.index();
        bool is_host = index == host.index();

        entrance[index] = enter_lobby(
            env, console,
            is_host ? boss_slot : 0,
            (HostingMode)(size_t)settings.MODE == HostingMode::HOST_ONLINE,
            console_stats[index].ore
        );
        if (entrance[index].isNull()){
            errors.fetch_add(1);
            return;
        }
    });
    if (errors.load(std::memory_order_acquire) != 0){
        env.run_in_parallel([&](ConsoleHandle& console){
            pbf_mash_button(console, BUTTON_B, 8 * TICKS_PER_SECOND);
        });
        return false;
    }

    env.run_in_parallel([&](ConsoleHandle& console){
        size_t index = console.index();
        GlobalState& state = state_tracker[index];
        bool is_host = index == host.index();

        //  Read boss.
        if (is_host){
            boss = read_boss_sprite(console);
            state.boss = boss;
        }

        //  Enter Code
        if (has_code){
            pbf_press_button(console, BUTTON_PLUS, 10, TICKS_PER_SECOND);
            enter_digits(console, 8, code);
            pbf_wait(console, 2 * TICKS_PER_SECOND);
            pbf_press_button(console, BUTTON_A, 10, TICKS_PER_SECOND);
        }
    });

    //  Open lobby.
    env.run_in_parallel([&](ConsoleHandle& console){
        //  If you start the raids at the same time, they won't find each other.
        if (console.index() != host.index()){
            pbf_wait(console, 3 * TICKS_PER_SECOND);
        }
        pbf_press_button(console, BUTTON_A, 10, TICKS_PER_SECOND);
    });

    auto time_limit = std::chrono::system_clock::now() +
        std::chrono::milliseconds(settings.LOBBY_WAIT_DELAY * 1000 / TICKS_PER_SECOND);

    AllJoinedTracker joined_tracker(env, env.consoles.size(), time_limit);

    //  Wait for all Switches to join.
    env.run_in_parallel([&](ConsoleHandle& console){
        //  Wait for a player to show up. This lets you ready up.
        size_t index = console.index();
        if (!wait_for_a_player(env, console, entrance[index], time_limit)){
            errors.fetch_add(1);
            return;
        }
    });
    if (abort_if_error(env, errors)){
        return false;
    }

    env.run_in_parallel([&](ConsoleHandle& console){
        //  Wait for all consoles to join.
        if (!joined_tracker.report_joined()){
            console.log("Not everyone was able to join.", COLOR_RED);
            errors.fetch_add(1);
            return;
        }
    });
    if (abort_if_error(env, errors)){
        return false;
    }

    env.run_in_parallel([&](ConsoleHandle& console){
        size_t index = console.index();
        if (!wait_for_all_join(env, console, entrance[index], env.consoles.size())){
            console.log("Switches joined into different raids.", COLOR_RED);
            errors.fetch_add(1);
            return;
        }
    });
    if (abort_if_error(env, errors)){
        return false;
    }

    send_raid_notification(
        env.program_info(),
        host,
        settings.NOTIFICATIONS,
        has_code, code,
        boss,
        path_stats, session_stats
    );

    //  Ready up and wait for lobby to be ready.
    env.run_in_parallel([&](ConsoleHandle& console){
        //  Ready up.
        env.wait_for(std::chrono::seconds(1));
        pbf_press_button(console, BUTTON_A, 10, TICKS_PER_SECOND);
        console.botbase().wait_for_all_requests();

        //  Wait
        size_t index = console.index();
        if (!wait_for_lobby_ready(env, console, entrance[index], env.consoles.size(), 4, time_limit)){
            errors.fetch_add(1);
            pbf_mash_button(console, BUTTON_B, 10 * TICKS_PER_SECOND);
            return;
        }
    });
    if (abort_if_error(env, errors)){
        return false;
    }

    env.run_in_parallel([&](ConsoleHandle& console){
        //  Start
        size_t index = console.index();
        if (!start_adventure(env, console, env.consoles.size(), entrance[index])){
            errors.fetch_add(1);
            pbf_mash_button(console, BUTTON_B, 10 * TICKS_PER_SECOND);
            return;
        }
    });
    if (abort_if_error(env, errors)){
        return false;
    }

    return true;
}



bool start_adventure(
    MultiSwitchProgramEnvironment& env,
    GlobalStateTracker& state_tracker,
    QImage entrance[4],
    ConsoleHandle& host, size_t boss_slot,
    HostingSettings& settings,
    const PathStats& path_stats,
    const StatsTracker& session_stats,
    ConsoleRuntime console_stats[4]
){
    switch ((HostingMode)(size_t)settings.MODE){
    case HostingMode::NOT_HOSTING:
        return start_raid_local(env, state_tracker, entrance, host, boss_slot, settings, console_stats);
    case HostingMode::HOST_LOCALLY:
    case HostingMode::HOST_ONLINE:
        return start_raid_host(
            env,
            state_tracker,
            entrance,
            host, boss_slot,
            settings,
            path_stats, session_stats,
            console_stats
        );
    }
    PA_THROW_StringException("Invalid mode enum.");
}










}
}
}
}
