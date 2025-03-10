/*  Device Functions
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Cpp/Exception.h"
#include "NintendoSwitch_Device.h"

namespace PokemonAutomation{
namespace NintendoSwitch{


uint32_t system_clock(const BotBaseContext& context){
    pabb_MsgAckRequestI32 response;
    context.issue_request_and_wait(
        DeviceRequest_system_clock()
    ).convert<PABB_MSG_ACK_REQUEST_I32>(response);
    return response.data;
}
void set_leds(const BotBaseContext& context, bool on){
    context.issue_request(DeviceRequest_set_leds(on));
}





}
}
