/*  Program Information
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_ProgramInfo_H
#define PokemonAutomation_ProgramInfo_H

#include <string>
#include <chrono>
#include <QString>

namespace PokemonAutomation{


struct ProgramInfo{
    std::string program_id;
    QString program_name;
    std::chrono::system_clock::time_point start_time;

    ProgramInfo(
        const std::string& module = "",
        std::chrono::system_clock::time_point p_start_time = std::chrono::system_clock::time_point::min()
    )
        : program_name(QString::fromStdString(module))
        , start_time(p_start_time)
    {}
    ProgramInfo(
        std::string p_program_id,
        QString category, QString display_name,
        std::chrono::system_clock::time_point p_start_time = std::chrono::system_clock::time_point::min()
    )
        : program_id(std::move(p_program_id))
        , program_name((category.isEmpty() ? "" : category + ": ") + display_name)
        , start_time(p_start_time)
    {}
};



}
#endif
