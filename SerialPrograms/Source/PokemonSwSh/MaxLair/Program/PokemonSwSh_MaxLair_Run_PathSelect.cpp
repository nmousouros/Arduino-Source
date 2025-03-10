/*  Max Lair Run Path Select
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "CommonFramework/Tools/ErrorDumper.h"
#include "CommonFramework/Tools/InterruptableCommands.h"
#include "CommonFramework/Inference/VisualInferenceSession.h"
#include "NintendoSwitch/Commands/NintendoSwitch_PushButtons.h"
#include "PokemonSwSh/MaxLair/Inference/PokemonSwSh_MaxLair_Detect_ItemSelectMenu.h"
#include "PokemonSwSh/MaxLair/Inference/PokemonSwSh_MaxLair_Detect_BattleMenu.h"
#include "PokemonSwSh/MaxLair/Inference/PokemonSwSh_MaxLair_Detect_PathSelect.h"
#include "PokemonSwSh/MaxLair/Inference/PokemonSwSh_MaxLair_Detect_EndBattle.h"
#include "PokemonSwSh/MaxLair/AI/PokemonSwSh_MaxLair_AI.h"
#include "PokemonSwSh_MaxLair_Run_ItemSelect.h"
#include "PokemonSwSh_MaxLair_Run_PathSelect.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{
namespace MaxLairInternal{


void run_path_select(
    ProgramEnvironment& env,
    ConsoleHandle& console,
    GlobalStateTracker& state_tracker
){
    size_t console_index = console.index();
    GlobalState& state = state_tracker[console_index];
    size_t player_index = state.find_player_index(console_index);

    PathReader reader(console, player_index);
    env.wait_for(std::chrono::milliseconds(500));

    QImage screen = console.video().snapshot();
    reader.read_sprites(console, state, screen);
    reader.read_hp(console, state, screen);

    if (state.wins == 0){
        reader.read_path(env, console, state);
    }else{
        reader.read_side(console, state, screen);
    }


    GlobalState inferred = state_tracker.synchronize(env, console, console_index);


    //  Select the path.
    std::vector<PathNode> path = select_path(console, inferred, player_index);
    uint8_t slot;
    if (path.empty()){
        console.log("No available paths due to read errors. Picking left-most path.", COLOR_RED);
        slot = 0;
    }else{
        slot = path[0].path_slot;
    }
    state.last_best_path = std::move(path);

    console.log("Choosing path " + std::to_string((int)slot) + ".", COLOR_PURPLE);

    for (uint8_t c = 0; c < slot; c++){
        pbf_press_dpad(console, DPAD_RIGHT, 10, 50);
    }
    pbf_press_button(console, BUTTON_A, 10, TICKS_PER_SECOND);
    console.botbase().wait_for_all_requests();
}



}
}
}
}
