/*  End Battle Detector
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Compiler.h"
#include "CommonFramework/ImageTools/SolidColorTest.h"
#include "CommonFramework/Inference/ImageTools.h"
#include "CommonFramework/Inference/BlackScreenDetector.h"
#include "PokemonBDSP_EndBattleDetector.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{




EndBattleWatcher::EndBattleWatcher(const ImageFloatBox& box, Color color)
    : m_color(color)
    , m_box(box)
    , m_has_been_black(false)
{}
void EndBattleWatcher::make_overlays(OverlaySet& items) const{
    items.add(m_color, m_box);
}
bool EndBattleWatcher::process_frame(
    const QImage& frame,
    std::chrono::system_clock::time_point timestamp
){
    return battle_is_over(frame);
}
bool EndBattleWatcher::battle_is_over(const QImage& frame){
    QImage image = extract_box(frame, m_box);
    ImageStats stats = image_stats(image);
    if (is_black(stats)){
        m_has_been_black = true;
        return false;
    }
    if (!m_has_been_black){
        return false;
    }
//    cout << stats.stddev.sum() << endl;
    if (stats.stddev.sum() < 20){
        return false;
    }
    return true;
}



}
}
}
