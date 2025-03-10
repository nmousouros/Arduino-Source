/*  Egg Fetcher
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonBDSP_EggFetcher_H
#define PokemonAutomation_PokemonBDSP_EggFetcher_H

#include "CommonFramework/Options/SimpleIntegerOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "NintendoSwitch/Options/TimeExpressionOption.h"
#include "NintendoSwitch/Options/GoHomeWhenDoneOption.h"
#include "NintendoSwitch/Framework/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonBDSP/Options/PokemonBDSP_ShortcutDirection.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{



class EggFetcher_Descriptor : public RunnableSwitchProgramDescriptor{
public:
    EggFetcher_Descriptor();
};




class EggFetcher : public SingleSwitchProgramInstance{
public:
    EggFetcher(const EggFetcher_Descriptor& descriptor);

    virtual std::unique_ptr<StatsTracker> make_stats() const override;
    virtual void program(SingleSwitchProgramEnvironment& env) override;


private:
    struct Stats;

private:
    GoHomeWhenDoneOption GO_HOME_WHEN_DONE;

    ShortcutDirection SHORTCUT;
    SimpleIntegerOption<uint16_t> MAX_FETCH_ATTEMPTS;
    TimeExpressionOption<uint16_t> TRAVEL_TIME_PER_FETCH;

    EventNotificationOption NOTIFICATION_STATUS_UPDATE;
    EventNotificationOption NOTIFICATION_PROGRAM_FINISH;
    EventNotificationsOption NOTIFICATIONS;
};




}
}
}
#endif
