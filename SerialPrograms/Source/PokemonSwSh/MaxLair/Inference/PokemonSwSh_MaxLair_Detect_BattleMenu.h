/*  Max Lair Detect Battle Menu
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonSwSh_MaxLair_Detect_BattleMenu_H
#define PokemonAutomation_PokemonSwSh_MaxLair_Detect_BattleMenu_H

#include "CommonFramework/Language.h"
#include "CommonFramework/Logging/Logger.h"
#include "CommonFramework/Tools/ProgramEnvironment.h"
#include "CommonFramework/Tools/VideoFeed.h"
#include "CommonFramework/Inference/VisualInferenceCallback.h"
#include "PokemonSwSh/MaxLair/Framework/PokemonSwSh_MaxLair_State.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{
namespace MaxLairInternal{


class BattleMenuDetector : public VisualInferenceCallback{
public:
    BattleMenuDetector();

    bool detect(const QImage& screen);

    virtual void make_overlays(OverlaySet& items) const override;
    virtual bool process_frame(
        const QImage& frame,
        std::chrono::system_clock::time_point timestamp
    ) override final;

    bool dmaxed() const{ return m_dmaxed; }
    bool cheer() const{ return m_cheer; }


private:
    ImageFloatBox m_icon_fight;
    ImageFloatBox m_icon_pokemon;
    ImageFloatBox m_icon_run;
    ImageFloatBox m_text_fight;
    ImageFloatBox m_text_pokemon;
    ImageFloatBox m_text_run;
//    ImageFloatBox m_info_left;
//    ImageFloatBox m_info_right;
    ImageFloatBox m_status0;
    ImageFloatBox m_status1;

    size_t m_trigger_count = 0;
    bool m_dmaxed = false;
    bool m_cheer = false;
};



class BattleMenuReader{
public:
    BattleMenuReader(VideoOverlay& overlay, Language language);

    std::set<std::string> read_opponent(
        Logger& logger,
        ProgramEnvironment& env,
        VideoFeed& feed
    ) const;
    std::set<std::string> read_opponent_in_summary(Logger& logger, const QImage& screen) const;

    std::string read_own_mon(Logger& logger, const QImage& screen) const;

    double read_opponent_hp(Logger& logger, const QImage& screen) const;
    double read_own_hp(Logger& logger, const QImage& screen) const;
    void read_hp(Logger& logger, const QImage& screen, Health health[4], size_t player_index);
    void read_own_pp(Logger& logger, const QImage& screen, int8_t pp[4]) const;
    bool can_dmax(const QImage& screen) const;

private:
    Language m_language;
    InferenceBoxScope m_opponent_name;
    InferenceBoxScope m_summary_opponent_name;
    InferenceBoxScope m_summary_opponent_types;

    InferenceBoxScope m_own_name;
    InferenceBoxScope m_own_sprite;

    InferenceBoxScope m_opponent_hp;
    InferenceBoxScope m_own_hp;

    InferenceBoxScope m_hp0;
    InferenceBoxScope m_hp1;
    InferenceBoxScope m_hp2;
    InferenceBoxScope m_sprite0;
    InferenceBoxScope m_sprite1;
    InferenceBoxScope m_sprite2;

    InferenceBoxScope m_pp0;
    InferenceBoxScope m_pp1;
    InferenceBoxScope m_pp2;
    InferenceBoxScope m_pp3;
    InferenceBoxScope m_dmax;
};



}
}
}
}
#endif
