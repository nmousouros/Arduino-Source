/*  Battle Menu Detector
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Compiler.h"
#include "CommonFramework/ImageTools/SolidColorTest.h"
#include "CommonFramework/ImageTools/ColorClustering.h"
#include "CommonFramework/Inference/ImageTools.h"
#include "PokemonBDSP_BattleMenuDetector.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{


BattleMenuDetector::BattleMenuDetector(BattleType battle_type, Color color)
    : m_color(color)
    , m_battle_type(battle_type)
    , m_left0_status(0.140, 0.922, 0.100, 0.010)
    , m_left1_status(0.140, 0.910, 0.100, 0.010)
    , m_right_status(0.405, 0.925, 0.100, 0.010)
    , m_ball_left   (0.890, 0.475, 0.02, 0.03)
    , m_ball_right  (0.960, 0.475, 0.03, 0.03)
    , m_menu_battle (0.817, 0.585 + 0 * 0.1075, 0.150, 0.070)
    , m_menu_pokemon(0.817, 0.585 + 1 * 0.1075, 0.150, 0.070)
    , m_menu_bag    (0.817, 0.585 + 2 * 0.1075, 0.150, 0.070)
    , m_menu_run    (0.817, 0.585 + 3 * 0.1075, 0.150, 0.070)
{}
void BattleMenuDetector::make_overlays(OverlaySet& items) const{
    items.add(m_color, m_left0_status);
    items.add(m_color, m_left1_status);
    items.add(m_color, m_right_status);
    items.add(m_color, m_ball_left);
    items.add(m_color, m_ball_right);
    items.add(m_color, m_menu_battle);
    items.add(m_color, m_menu_pokemon);
    items.add(m_color, m_menu_bag);
    items.add(m_color, m_menu_run);
}
bool BattleMenuDetector::detect(const QImage& screen) const{
    {
        bool left0 = is_white(extract_box(screen, m_left0_status));
        bool left1 = is_white(extract_box(screen, m_left1_status));
        bool right = is_white(extract_box(screen, m_right_status));
        if (!left0 && !left1 && !right){
            return false;
        }
    }

    if (m_battle_type == BattleType::STARTER){
        if (!cluster_fit_2(
            extract_box(screen, m_menu_battle),
            qRgb(200, 70, 85), 0.8,
            qRgb(240, 170, 180), 0.2,
            0.2, 80, 0.1
        )){
            return false;
        }
        return true;
    }


    if (m_battle_type == BattleType::WILD){
        if (!is_white(extract_box(screen, m_ball_left))){
//            cout << "Not white" << endl;
            return false;
        }
        if (!is_white(extract_box(screen, m_ball_right))){
//            cout << "Not white" << endl;
            return false;
        }
    }

#if 0
    if (!cluster_fit_2(
        extract_box(screen, m_menu_battle),
        qRgb(200, 70, 85), 0.8,
        qRgb(240, 170, 180), 0.2,
        0.2, 80, 0.1
    )){
//        cout << "Not Battle" << endl;
        return false;
    }
#else
    {
        ImageStats stats = image_stats(extract_box(screen, m_menu_battle));
//        cout << stats.average << stats.stddev << endl;
        double stddev = stats.stddev.sum();
        if (stddev < 30 || stddev > 150){
            return false;
        }
        double average = stats.average.sum();
        FloatPixel actual = stats.average / average;
        if (euclidean_distance(actual, {0.541961, 0.231317, 0.226722}) > 0.2){
            return false;
        }
    }
#endif
//    cout << "Start Checking Pokemon" << endl;
#if 0
    if (!cluster_fit_2(
        extract_box(screen, m_menu_pokemon),
        qRgb(110, 200, 90), 0.85,
        qRgb(180, 240, 170), 0.15,
        0.2, 80, 0.1
    )){
        cout << "Not Pokemon" << endl;
        return false;
    }
#else
    {
        ImageStats stats = image_stats(extract_box(screen, m_menu_pokemon));
//        cout << stats.average << stats.stddev << endl;
        double stddev = stats.stddev.sum();
        if (stddev < 30 || stddev > 150){
            return false;
        }
        double average = stats.average.sum();
        FloatPixel actual = stats.average / average;
        if (euclidean_distance(actual, {0.255944, 0.582771, 0.161285}) > 0.2){
            return false;
        }
    }
#endif
    //    cout << "Start Checking bag" << endl;
#if 0
    if (!cluster_fit_2(
        extract_box(screen, m_menu_bag),
        qRgb(210, 190, 70), 0.85,
        qRgb(230, 220, 150), 0.15,
        0.2, 70, 0.1
    )){
        cout << "Not Bag" << endl;
        return false;
    }
#else
    {
        ImageStats stats = image_stats(extract_box(screen, m_menu_bag));
//        cout << stats.average << stats.stddev << endl;
        double stddev = stats.stddev.sum();
        if (stddev < 30 || stddev > 100){
            return false;
        }
        double average = stats.average.sum();
        FloatPixel actual = stats.average / average;
        if (euclidean_distance(actual, {0.485857, 0.414946, 0.099197}) > 0.2){
            return false;
        }
    }
#endif
#if 0
    if (!cluster_fit_2(
        extract_box(screen, m_menu_run),
        qRgb(120, 120, 210), 0.9,
        qRgb(190, 190, 240), 0.1,
        0.2, 70, 0.1
    )){
//        cout << "Not Run" << endl;
        return false;
    }
#else
    {
        ImageStats stats = image_stats(extract_box(screen, m_menu_run));
//        cout << stats.average << stats.stddev << endl;
        double stddev = stats.stddev.sum();
        if (stddev < 30 || stddev > 100){
            return false;
        }
        double average = stats.average.sum();
        FloatPixel actual = stats.average / average;
        if (euclidean_distance(actual, {0.250977, 0.269868, 0.479155}) > 0.2){
            return false;
        }
    }
#endif

    return true;
}



void BattleMenuWatcher::make_overlays(OverlaySet& items) const{
    BattleMenuDetector::make_overlays(items);
}
bool BattleMenuWatcher::process_frame(
    const QImage& frame,
    std::chrono::system_clock::time_point timestamp
){
    //  Need 5 consecutive successful detections.
    if (!detect(frame)){
        m_trigger_count = 0;
        return false;
    }
    m_trigger_count++;
    return m_trigger_count >= 5;
}




}
}
}
