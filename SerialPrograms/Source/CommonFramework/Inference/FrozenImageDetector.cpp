/*  Frozen Screen Detector
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "CommonFramework/ImageMatch/ImageDiff.h"
#include "FrozenImageDetector.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{


FrozenImageDetector::FrozenImageDetector(std::chrono::milliseconds timeout, double rmsd_threshold)
    : m_color(COLOR_CYAN)
    , m_box(0.0, 0.0, 1.0, 1.0)
    , m_timeout(timeout)
    , m_rmsd_threshold(rmsd_threshold)
{}
FrozenImageDetector::FrozenImageDetector(
    Color color, const ImageFloatBox& box,
    std::chrono::milliseconds timeout, double rmsd_threshold
)
    : m_color(color)
    , m_box(box)
    , m_timeout(timeout)
    , m_rmsd_threshold(rmsd_threshold)
{}
void FrozenImageDetector::make_overlays(OverlaySet& set) const{
    set.add(m_color, m_box);
}
bool FrozenImageDetector::process_frame(
    const QImage& frame,
    std::chrono::system_clock::time_point timestamp
){
    QImage image = extract_box(frame, m_box);
    if (m_last_delta.size() != image.size()){
        m_timestamp = timestamp;
        m_last_delta = image;
        return false;
    }

    double rmsd = ImageMatch::pixel_RMSD(m_last_delta, image);
//    cout << "rmsd = " << rmsd << endl;
    if (rmsd > m_rmsd_threshold){
        m_timestamp = timestamp;
        m_last_delta = image;
        return false;
    }

    return timestamp - m_timestamp > m_timeout;
//    return false;
}


}
