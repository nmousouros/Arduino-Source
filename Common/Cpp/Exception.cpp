/*  Exception
 * 
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 * 
 */

#include "Exception.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{


StringException::StringException(const char* type, const char* location, std::string message)
    : m_message(std::move(message))
{
    m_full_body = type;
    m_full_body += ":\r\n\r\n";
    m_full_body += "Location: ";
    m_full_body += location;
    m_full_body += "\r\n\r\n";
    m_full_body += m_message;
    m_full_body += "\r\n\r\n";

//    cout << m_full_body << endl;
}



FileException::FileException(const char* location, std::string message, const std::string& file)
    : StringException("FileException", location, std::move(message))
{
    m_full_body += "\r\n\r\n";
    m_full_body += file;

    m_message += "\r\n\r\n";
    m_message += file;
}


}

