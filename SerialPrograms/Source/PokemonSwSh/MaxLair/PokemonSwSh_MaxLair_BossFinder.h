/*  Max Lair (Boss Finder)
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonSwSh_MaxLair_BossFinder_H
#define PokemonAutomation_PokemonSwSh_MaxLair_BossFinder_H

#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "NintendoSwitch/Options/StartInGripMenuOption.h"
#include "NintendoSwitch/Options/GoHomeWhenDoneOption.h"
#include "NintendoSwitch/Framework/NintendoSwitch_MultiSwitchProgram.h"
#include "PokemonSwSh/Options/PokemonSwSh_DateToucher.h"
#include "Options/PokemonSwSh_MaxLair_Options.h"
#include "Options/PokemonSwSh_MaxLair_Options_Consoles.h"
#include "Options/PokemonSwSh_MaxLair_Options_Hosting.h"
#include "Options/PokemonSwSh_MaxLair_Options_BossAction.h"


namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{


class MaxLairBossFinder_Descriptor : public MultiSwitchProgramDescriptor{
public:
    MaxLairBossFinder_Descriptor();
};


class MaxLairBossFinder : public MultiSwitchProgramInstance{
public:
    MaxLairBossFinder(const MaxLairBossFinder_Descriptor& descriptor);

    virtual QString check_validity() const override;
    virtual void update_active_consoles() override;

    virtual std::unique_ptr<StatsTracker> make_stats() const override;
    virtual void program(MultiSwitchProgramEnvironment& env) override;


private:
    StartInGripOrGameOption START_IN_GRIP_MENU;
    GoHomeWhenDoneOption GO_HOME_WHEN_DONE;

    MaxLairInternal::Consoles CONSOLES;
    MaxLairInternal::BossActionOption BOSS_LIST;
    MaxLairInternal::HostingSettings HOSTING;

    TouchDateIntervalOption TOUCH_DATE_INTERVAL;

    EventNotificationOption NOTIFICATION_STATUS;
    EventNotificationOption NOTIFICATION_SHINY;
    EventNotificationOption NOTIFICATION_PROGRAM_FINISH;
    EventNotificationsOption NOTIFICATIONS;
};



}
}
}
#endif
