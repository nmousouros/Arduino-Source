/*  Frozen Screen Detector
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_CommonFramework_FrozenImageDetector_H
#define PokemonAutomation_CommonFramework_FrozenImageDetector_H

#include "CommonFramework/Inference/VisualInferenceCallback.h"

namespace PokemonAutomation{


class FrozenImageDetector : public VisualInferenceCallback{
public:
    FrozenImageDetector(std::chrono::milliseconds timeout, double rmsd_threshold);
    FrozenImageDetector(
        Color color, const ImageFloatBox& box,
        std::chrono::milliseconds timeout, double rmsd_threshold
    );

    virtual void make_overlays(OverlaySet& items) const override;
    virtual bool process_frame(
        const QImage& frame,
        std::chrono::system_clock::time_point timestamp
   ) override;

private:
    Color m_color;
    ImageFloatBox m_box;
    std::chrono::milliseconds m_timeout;
    double m_rmsd_threshold;
    std::chrono::system_clock::time_point m_timestamp;
    QImage m_last_delta;
};



}
#endif
