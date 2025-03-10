/*  Start Battle Detector
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonBDSP_StartBattleDetector_H
#define PokemonAutomation_PokemonBDSP_StartBattleDetector_H

#include <functional>
#include <chrono>
#include <thread>
#include "CommonFramework/Logging/Logger.h"
#include "CommonFramework/Tools/VideoFeed.h"
#include "CommonFramework/Tools/ProgramEnvironment.h"
#include "CommonFramework/Inference/VisualInferenceCallback.h"
#include "PokemonBDSP/Inference/PokemonBDSP_DialogDetector.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{




class StartBattleDetector : public VisualInferenceCallback{
public:
    StartBattleDetector(VideoOverlay& overlay);

    bool detect(const QImage& frame);

    virtual void make_overlays(OverlaySet& items) const override;
    virtual bool process_frame(
        const QImage& frame,
        std::chrono::system_clock::time_point timestamp
    ) override final;

private:
    ImageFloatBox m_screen_box;
    BattleDialogDetector m_dialog;
};


class StartBattleMenuOverlapDetector : public VisualInferenceCallback{
public:
    StartBattleMenuOverlapDetector(VideoOverlay& overlay);

    bool detected() const{ return m_battle_detected.load(std::memory_order_acquire); }
    bool detect(const QImage& frame);

    virtual void make_overlays(OverlaySet& items) const override;
    virtual bool process_frame(
        const QImage& frame,
        std::chrono::system_clock::time_point timestamp
    ) override final;

private:
    ImageFloatBox m_left;
    ImageFloatBox m_right;
    std::atomic<bool> m_battle_detected;
};







}
}
}
#endif

