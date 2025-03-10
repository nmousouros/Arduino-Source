/*  Message Logger
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Microcontroller/MessageProtocol.h"
#include "Common/NintendoSwitch/NintendoSwitch_Protocol_PushButtons.h"
#include "ClientSource/Libraries/MessageConverter.h"
#include "BotBaseMessage.h"
#include "MessageLogger.h"

namespace PokemonAutomation{


//void MessageLogger::log(std::string msg){
//    log(msg);
//}
void MessageLogger::on_send(const BotBaseMessage& message, bool is_retransmit){
    bool print = false;
    do{
        if (is_retransmit){
            print = true;
        }
        if (PABB_MSG_IS_REQUEST(message.type)){
            print = true;
        }
        if (PABB_MSG_IS_COMMAND(message.type)){
            print = true;
        }
        if (message.type == PABB_MSG_REQUEST_CLOCK){
            print = false;
        }
        if (message.type == PABB_MSG_CONTROLLER_STATE){
            print = false;
        }

        if (m_log_everything.load(std::memory_order_relaxed)){
            print = true;
        }

    }while (false);
    if (!print){
        return;
    }
    std::string str = message_to_string(message);
    if (str.empty()){
        return;
    }
    if (is_retransmit){
        log("Re-Send: " + str);
    }else{
        log("Sending: " + str);
    }
}
void MessageLogger::on_recv(const BotBaseMessage& message){
    bool print = false;
    do{
        if (PABB_MSG_IS_ERROR(message.type)){
            print = true;
        }
        if (PABB_MSG_IS_INFO(message.type)){
            print = true;
        }

        if (m_log_everything.load(std::memory_order_relaxed)){
            print = true;
        }

    }while (false);
    if (!print){
        return;
    }
    log("Receive: " + message_to_string(message));
}




}
