/*  Box Detector
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Compiler.h"
#include "CommonFramework/ImageTools/ImageStats.h"
#include "CommonFramework/ImageTools/SolidColorTest.h"
#include "PokemonBDSP_BoxDetector.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{


BoxDetector::BoxDetector(Color color)
    : m_color(color)
    , m_left(0.265, 0.09, 0.03, 0.04)
    , m_right(0.650, 0.09, 0.03, 0.04)
    , m_bottom(0.02, 0.97, 0.10, 0.02)
    , m_row(0.26, 0.20, 0.40, 0.10)
{}
void BoxDetector::make_overlays(OverlaySet& items) const{
    items.add(m_color, m_left);
    items.add(m_color, m_right);
    items.add(m_color, m_bottom);
    items.add(m_color, m_row);
}
bool BoxDetector::detect(const QImage& screen) const{
    ImageStats left = image_stats(extract_box(screen, m_left));
//    cout << left.average << left.stddev << endl;
    if (!is_solid(left, {0.274119, 0.355324, 0.370557})){
        return false;
    }
    ImageStats right = image_stats(extract_box(screen, m_right));
//    cout << right.average << right.stddev << endl;
    if (!is_solid(right, {0.274119, 0.355324, 0.370557})){
        return false;
    }
    ImageStats bottom = image_stats(extract_box(screen, m_bottom));
//    cout << bottom.average << bottom.stddev << endl;
    if (!is_solid(bottom, {0.190353, 0.327458, 0.482189})){
        return false;
    }
    ImageStats row = image_stats(extract_box(screen, m_row));
//    cout << row.average << row.stddev << endl;
    if (row.stddev.sum() < 50){
        return false;
    }
    return true;
}



void BoxWatcher::make_overlays(OverlaySet& items) const{
    BoxDetector::make_overlays(items);
}
bool BoxWatcher::process_frame(
    const QImage& frame,
    std::chrono::system_clock::time_point timestamp
){
    return detect(frame);
}




}
}
}
