/*  Game Entry
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "CommonFramework/Inference/InferenceException.h"
#include "CommonFramework/Inference/VisualInferenceRoutines.h"
#include "NintendoSwitch/Commands/NintendoSwitch_PushButtons.h"
#include "PokemonBDSP/PokemonBDSP_Settings.h"
#include "PokemonBDSP/Inference/PokemonBDSP_MenuDetector.h"
#include "PokemonBDSP/Inference/BoxSystem/PokemonBDSP_BoxDetector.h"
#include "PokemonBDSP_GameNavigation.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{


//  Non-Feedback

void save_game(const BotBaseContext& context){
    pbf_press_button(context, BUTTON_X, 10, GameSettings::instance().OVERWORLD_TO_MENU_DELAY);
    pbf_press_button(context, BUTTON_R, 10, 2 * TICKS_PER_SECOND);
    pbf_press_button(context, BUTTON_ZL, 10, 5 * TICKS_PER_SECOND);
}
void menu_to_box(const BotBaseContext& context){
    uint16_t MENU_TO_POKEMON_DELAY = GameSettings::instance().MENU_TO_POKEMON_DELAY;
    pbf_mash_button(context, BUTTON_ZL, 30);
    if (MENU_TO_POKEMON_DELAY > 30){
        pbf_wait(context, MENU_TO_POKEMON_DELAY - 30);
    }

    pbf_press_button(context, BUTTON_R, 20, GameSettings::instance().POKEMON_TO_BOX_DELAY0);
}
void overworld_to_box(const BotBaseContext& context){
    pbf_press_button(context, BUTTON_X, 20, GameSettings::instance().OVERWORLD_TO_MENU_DELAY);
//    pbf_press_button(context, BUTTON_ZL, 20, GameSettings::instance().MENU_TO_POKEMON_DELAY);

    menu_to_box(context);
}
void box_to_overworld(const BotBaseContext& context){
    //  There are two states here which need to be merged:
    //      1.  The depositing column was empty. The party has been swapped and
    //          it's sitting in the box with no held pokemon.
    //      2.  The depositing column was not empty. The party swap failed and
    //          it's sitting in the box holding on the party pokemon.
    //
    //  Double press B quickly here to back out of the box.
    //  In state (1):   The 1st B will begin back out of the box. The 2nd B will
    //                  be swallowed by the animation.
    //  In state (2):   The 1st B will drop the party pokemon. The 2nd B will
    //                  back out of the box.

    pbf_press_button(context, BUTTON_B, 20, 30);
    pbf_press_button(context, BUTTON_B, 20, GameSettings::instance().BOX_TO_POKEMON_DELAY);

    pbf_press_button(context, BUTTON_B, 20, GameSettings::instance().POKEMON_TO_MENU_DELAY);
    pbf_press_button(context, BUTTON_X, 20, GameSettings::instance().MENU_TO_OVERWORLD_DELAY);
}



//  Feedback

void overworld_to_menu(ProgramEnvironment& env, ConsoleHandle& console){
    pbf_press_button(console, BUTTON_X, 20, 105);
    console.botbase().wait_for_all_requests();
    {
        MenuWatcher detector;
        int ret = wait_until(
            env, console, std::chrono::seconds(10),
            { &detector }
        );
        if (ret < 0){
            PA_THROW_InferenceException(console, "Menu not detected after 10 seconds.");
        }
        console.log("Detected menu.");
    }
}

void save_game(ProgramEnvironment& env, ConsoleHandle& console){
    overworld_to_menu(env, console);
    pbf_press_button(console, BUTTON_R, 10, 2 * TICKS_PER_SECOND);
    pbf_press_button(console, BUTTON_ZL, 10, 5 * TICKS_PER_SECOND);
}

void overworld_to_box(ProgramEnvironment& env, ConsoleHandle& console){
    //  Open menu.
    overworld_to_menu(env, console);

    //  Enter Pokemon
    uint16_t MENU_TO_POKEMON_DELAY = GameSettings::instance().MENU_TO_POKEMON_DELAY;
#if 0
//    pbf_mash_button(console, BUTTON_ZL, 30);
    if (MENU_TO_POKEMON_DELAY > 30){
        pbf_wait(console, MENU_TO_POKEMON_DELAY - 30);
    }
#else
    pbf_press_button(console, BUTTON_ZL, 20, MENU_TO_POKEMON_DELAY);
#endif

    //  Enter box system.
    pbf_press_button(console, BUTTON_R, 20, 105);
    console.botbase().wait_for_all_requests();
    {
        BoxWatcher detector;
        int ret = wait_until(
            env, console, std::chrono::seconds(10),
            { &detector }
        );
        if (ret < 0){
            PA_THROW_InferenceException(console, "Box system not detected after 10 seconds.");
        }
        console.log("Detected box system.");
    }
    env.wait_for(std::chrono::milliseconds(500));
}
void box_to_overworld(ProgramEnvironment& env, ConsoleHandle& console){
    //  There are two states here which need to be merged:
    //      1.  The depositing column was empty. The party has been swapped and
    //          it's sitting in the box with no held pokemon.
    //      2.  The depositing column was not empty. The party swap failed and
    //          it's sitting in the box holding on the party pokemon.
    //
    //  Double press B quickly here to back out of the box.
    //  In state (1):   The 1st B will begin back out of the box. The 2nd B will
    //                  be swallowed by the animation.
    //  In state (2):   The 1st B will drop the party pokemon. The 2nd B will
    //                  back out of the box.
    pbf_press_button(console, BUTTON_B, 20, 30);
    pbf_press_button(console, BUTTON_B, 20, GameSettings::instance().BOX_TO_POKEMON_DELAY);

    //  To menu.
    pbf_press_button(console, BUTTON_B, 20, 105);
    console.botbase().wait_for_all_requests();
    {
        MenuWatcher detector;
        int ret = wait_until(
            env, console, std::chrono::seconds(10),
            { &detector }
        );
        if (ret < 0){
            PA_THROW_InferenceException(console, "Menu not detected after 10 seconds.");
        }
        console.log("Detected menu.");
    }

    //  To overworld.
    pbf_press_button(console, BUTTON_X, 20, GameSettings::instance().MENU_TO_OVERWORLD_DELAY);
}





}
}
}
