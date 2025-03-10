/*  Battle Dialog Detector
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonSwSh_BattleDialogDetector_H
#define PokemonAutomation_PokemonSwSh_BattleDialogDetector_H

#include "CommonFramework/Inference/VisualDetector.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{



class BattleDialogDetector : public StaticScreenDetector{
public:
    BattleDialogDetector(Color color = COLOR_RED);

    virtual void make_overlays(OverlaySet& items) const override;
    virtual bool detect(const QImage& screen) const override;

private:
    Color m_color;
    ImageFloatBox m_bottom;
    ImageFloatBox m_left;
    ImageFloatBox m_right;
};



}
}
}
#endif
