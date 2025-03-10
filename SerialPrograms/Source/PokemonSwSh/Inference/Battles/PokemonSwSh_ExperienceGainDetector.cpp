/*  Experience Gain Detector
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Compiler.h"
#include "CommonFramework/ImageTools/SolidColorTest.h"
#include "PokemonSwSh_ExperienceGainDetector.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{



ExperienceGainDetector::ExperienceGainDetector(Color color)
    : m_color(color)
    , m_dialog(color)
    , m_rows(6)
{
    const double SHIFT_X = 0.0325;
    const double SHIFT_Y = 0.126;
    for (size_t c = 0; c < 6; c++){
        m_rows.emplace_back(
            std::piecewise_construct,
            std::forward_as_tuple(0.255            , 0.04 + c*SHIFT_Y, 0.18 - c*SHIFT_X, 0.08),
            std::forward_as_tuple(0.475 - c*SHIFT_X, 0.04 + c*SHIFT_Y, 0.05            , 0.08)
        );
    }
}

void ExperienceGainDetector::make_overlays(OverlaySet& items) const{
    m_dialog.make_overlays(items);
    for (const auto& item : m_rows){
        items.add(m_color, item.first);
        items.add(m_color, item.second);
    }
}
bool ExperienceGainDetector::detect(const QImage& screen) const{
    if (!m_dialog.detect(screen)){
        return false;
    }

    for (auto& row : m_rows){
        ImageStats stats0 = image_stats(extract_box(screen, row.first));
//        cout << stats0.average << " " << stats0.stddev << endl;
        if (!is_grey(stats0, 400, 1000)){
            return false;
        }
        ImageStats stats1 = image_stats(extract_box(screen, row.second));
//        cout << stats1.average << " " << stats1.stddev << endl;
        if (!is_white(stats1)){
            return false;
        }
        if (stats0.average.sum() > stats1.average.sum()){
            return false;
        }
    }

    return true;
}


void ExperienceGainWatcher::make_overlays(OverlaySet& items) const{
    ExperienceGainDetector::make_overlays(items);
}
bool ExperienceGainWatcher::process_frame(
    const QImage& frame,
    std::chrono::system_clock::time_point timestamp
){
    return detect(frame);
}




}
}
}
