/*  Max Lair Run Entrance
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonSwSh_MaxLair_Run_Entrance_H
#define PokemonAutomation_PokemonSwSh_MaxLair_Run_Entrance_H

#include "CommonFramework/Tools/ProgramEnvironment.h"
#include "CommonFramework/Tools/ConsoleHandle.h"
#include "PokemonSwSh/MaxLair/Framework/PokemonSwSh_MaxLair_StateTracker.h"
#include "PokemonSwSh/MaxLair/Framework/PokemonSwSh_MaxLair_StateMachine.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{
namespace MaxLairInternal{


void run_entrance(
    AdventureRuntime& runtime,
    ProgramEnvironment& env,
    ConsoleHandle& console, bool save_path,
    GlobalStateTracker& state_tracker
);



}
}
}
}
#endif
