/*  Catch Screen Tracker
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Cpp/Exception.h"
#include "CommonFramework/Tools/ErrorDumper.h"
#include "CommonFramework/Inference/VisualInferenceRoutines.h"
#include "NintendoSwitch/Commands/NintendoSwitch_PushButtons.h"
#include "PokemonSwSh/MaxLair/Options/PokemonSwSh_MaxLair_Options.h"
#include "PokemonSwSh/MaxLair/Inference/PokemonSwSh_MaxLair_Detect_EndBattle.h"
#include "PokemonSwSh_MaxLair_CatchScreenTracker.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{
namespace MaxLairInternal{


CaughtPokemonScreen::CaughtPokemonScreen(ProgramEnvironment& env, ConsoleHandle& console)
    : m_env(env)
    , m_console(console)
    , m_total(count_catches(console, console.video().snapshot()))
{
    if (m_total == 0 || m_total > 4){
        console.log("Detected " + std::to_string(m_total) + " catches. Something is wrong.", COLOR_RED);
    }
}

size_t CaughtPokemonScreen::total() const{
    return m_total;
}
const CaughtPokemon& CaughtPokemonScreen::operator[](size_t position) const{
    return m_mons[position];
}
bool CaughtPokemonScreen::current_position() const{
    return m_current_position;
}
bool CaughtPokemonScreen::is_summary() const{
    return m_in_summary;
}

void CaughtPokemonScreen::enter_summary(){
    SummaryShinySymbolDetector detector(m_console, m_console);
    if (m_in_summary){
        //  Make sure we're actually in the summary screen.
        process_detection(detector.wait_for_detection(m_env, m_console));
        return;
    }

    pbf_press_button(m_console, BUTTON_A, 10, 100);
    pbf_press_dpad(m_console, DPAD_DOWN, 10, 50);
    pbf_press_button(m_console, BUTTON_A, 10, 0);
    m_console.botbase().wait_for_all_requests();

    Detection detection = detector.wait_for_detection(m_env, m_console);
    m_in_summary = true;
    process_detection(detection);
}
void CaughtPokemonScreen::leave_summary(){
    if (!m_in_summary){
        return;
    }

    //  Make sure we're actually in the summary screen.
    SummaryShinySymbolDetector detector(m_console, m_console);
    process_detection(detector.wait_for_detection(m_env, m_console));

    pbf_press_button(m_console, BUTTON_B, 10, TICKS_PER_SECOND);

    PokemonCaughtMenuDetector caught_menu;

    int result = wait_until(
        m_env, m_console,
        std::chrono::seconds(10),
        { &caught_menu }
    );

    switch (result){
    case 0:
        pbf_wait(m_console, 125);
        m_console.botbase().wait_for_all_requests();
        break;
    default:
        m_console.log("Failed to detect caught menu.", COLOR_RED);
        dump_image(m_console, m_env.program_info(), "CaughtMenu", m_console.video().snapshot());
        PA_THROW_StringException("Failed to detect caught menu.");
    }

    m_in_summary = false;
}
void CaughtPokemonScreen::scroll_down(){
    pbf_press_dpad(m_console, DPAD_DOWN, 10, TICKS_PER_SECOND);
    m_console.botbase().wait_for_all_requests();
    m_current_position++;
    if (m_current_position >= m_total){
        m_current_position = 0;
    }
    if (m_in_summary){
        SummaryShinySymbolDetector detector(m_console, m_console);
        Detection detection = detector.wait_for_detection(m_env, m_console);
        process_detection(detection);
    }
}
void CaughtPokemonScreen::scroll_to(size_t position){
    while (m_current_position != position){
        scroll_down();
    }
}
void CaughtPokemonScreen::process_detection(Detection detection){
    CaughtPokemon& mon = m_mons[m_current_position];
    switch (detection){
    case SummaryShinySymbolDetector::Detection::NO_DETECTION:
        dump_image(m_console, m_env.program_info(), "SummaryScreen", m_console.video().snapshot());
        PA_THROW_StringException("Failed to detect summary screen.");
    case SummaryShinySymbolDetector::Detection::NOT_SHINY:
        if (!mon.read){
            m_console.log("Not shiny.", COLOR_BLUE);
            mon.shiny = false;
            mon.read = true;
        }else if (mon.shiny){
            dump_image(m_console, m_env.program_info(), "InconsistentShiny", m_console.video().snapshot());
            PA_THROW_StringException("Fatal Inconsistency: Expected to see a non-shiny.");
        }
        break;
    case SummaryShinySymbolDetector::Detection::SHINY:
        if (!mon.read){
            m_console.log("Found shiny!", COLOR_BLUE);
            mon.shiny = true;
            mon.read = true;
        }else if (!mon.shiny){
            dump_image(m_console, m_env.program_info(), "InconsistentShiny", m_console.video().snapshot());
            PA_THROW_StringException("Fatal Inconsistency: Expected to see a shiny.");
        }
        break;
    }
}



}
}
}
}
