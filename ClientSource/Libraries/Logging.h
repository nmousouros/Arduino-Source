/*  Pokemon Automation Bot Base - Client Example
 * 
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 * 
 */

#ifndef PokemonAutomation_Logging_H
#define PokemonAutomation_Logging_H

#include <string>
#include <sstream>

namespace PokemonAutomation{


void log(const std::stringstream& ss);
void log(const std::string& msg);

std::string current_time();



}
#endif
