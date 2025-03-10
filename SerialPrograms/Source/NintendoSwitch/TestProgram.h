/*  Test Program
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonSwSh_TestProgram_H
#define PokemonAutomation_PokemonSwSh_TestProgram_H

#include "CommonFramework/Options/BatchOption/BatchOption.h"
#include "CommonFramework/Options/BooleanCheckBoxOption.h"
#include "CommonFramework/Options/EnumDropdownOption.h"
#include "CommonFramework/Options/StringSelectOption.h"
#include "CommonFramework/Options/EditableTableOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "CommonFramework/OCR/OCR_LanguageOptionOCR.h"
#include "NintendoSwitch/Framework/NintendoSwitch_MultiSwitchProgram.h"
#include "Pokemon/Options/Pokemon_BallSelectOption.h"
#include "PokemonSwSh/Options/EncounterFilter/PokemonSwSh_EncounterFilterOption.h"
#include "PokemonSwSh/Inference/PokemonSwSh_IVCheckerReader.h"
#include "CommonFramework/Tools/StatsTracking.h"

namespace PokemonAutomation{
namespace NintendoSwitch{

//using namespace PokemonSwSh;





class TestProgram_Descriptor : public MultiSwitchProgramDescriptor{
public:
    TestProgram_Descriptor();
};


class TestProgram : public MultiSwitchProgramInstance{
public:
    TestProgram(const TestProgram_Descriptor& descriptor);

//    std::unique_ptr<StatsTracker> make_stats() const override{
//        return std::unique_ptr<StatsTracker>(new StatsTracker());
//    }
    virtual void program(MultiSwitchProgramEnvironment& env) override;

private:
    struct Stats : public StatsTracker{
        Stats()
            : m_resets(m_stats["Resets"])
        {
            m_display_order.emplace_back("Resets");
        }
        std::atomic<uint64_t>& m_resets;
    };

private:
    OCR::LanguageOCR LANGUAGE;
//    ProgramNotificationTable TABLE;
};



}
}
#endif

