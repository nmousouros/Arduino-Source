/*  Max Lair Detect Battle Menu
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <cmath>
#include "Common/Compiler.h"
#include "Common/Cpp/PrettyPrint.h"
#include "CommonFramework/Tools/ErrorDumper.h"
#include "CommonFramework/ImageTools/SolidColorTest.h"
#include "CommonFramework/ImageTools/ColorClustering.h"
#include "CommonFramework/OCR/OCR_Filtering.h"
#include "PokemonSwSh/Resources/PokemonSwSh_MaxLairDatabase.h"
#include "PokemonSwSh/Inference/PokemonSwSh_TypeSymbolFinder.h"
#include "PokemonSwSh/MaxLair/Options/PokemonSwSh_MaxLair_Options.h"
#include "PokemonSwSh_MaxLair_Detect_PokemonReader.h"
#include "PokemonSwSh_MaxLair_Detect_HPPP.h"
#include "PokemonSwSh_MaxLair_Detect_BattleMenu.h"

#include <iostream>
using std::cout;
using std::endl;

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{
namespace MaxLairInternal{


BattleMenuDetector::BattleMenuDetector()
    : m_icon_fight  (0.923, 0.576 + 1 * 0.1075, 0.05, 0.080)
    , m_icon_pokemon(0.923, 0.576 + 2 * 0.1075, 0.05, 0.080)
    , m_icon_run    (0.923, 0.576 + 3 * 0.1075, 0.05, 0.080)
    , m_text_fight  (0.830, 0.576 + 1 * 0.1075, 0.08, 0.080)
    , m_text_pokemon(0.830, 0.576 + 2 * 0.1075, 0.08, 0.080)
    , m_text_run    (0.830, 0.576 + 3 * 0.1075, 0.08, 0.080)
//    , m_info_left   (0.907, 0.500, 0.02, 0.03)
//    , m_info_right  (0.970, 0.500, 0.02, 0.03)
    , m_status0     (0.280, 0.870, 0.015, 0.030)
    , m_status1     (0.165, 0.945, 0.100, 0.020)
{}
void BattleMenuDetector::make_overlays(OverlaySet& items) const{
    items.add(COLOR_YELLOW, m_icon_fight);
    items.add(COLOR_YELLOW, m_icon_pokemon);
    items.add(COLOR_YELLOW, m_icon_run);
    items.add(COLOR_YELLOW, m_text_fight);
    items.add(COLOR_YELLOW, m_text_pokemon);
    items.add(COLOR_YELLOW, m_text_run);
    items.add(COLOR_YELLOW, m_status0);
    items.add(COLOR_YELLOW, m_status1);
}
bool BattleMenuDetector::process_frame(
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


bool BattleMenuDetector::detect(const QImage& screen){
    bool fight;

    fight = false;
    fight |= !fight && cluster_fit_2(
        extract_box(screen, m_text_fight),
        qRgb(0, 0, 0), 0.9,
        qRgb(255, 255, 255), 0.1,
        0.2, 50, 0.1
    );
    fight |= !fight && cluster_fit_2(
        extract_box(screen, m_text_fight),
        qRgb(0, 0, 0), 0.1,
        qRgb(255, 255, 255), 0.9,
        0.2, 50, 0.1
    );
    if (!fight){
        return false;
    }

    fight = false;
    fight |= !fight && cluster_fit_2(
        extract_box(screen, m_text_pokemon),
        qRgb(0, 0, 0), 0.1,
        qRgb(255, 255, 255), 0.9,
        0.2, 50, 0.1
    );
    fight |= !fight && cluster_fit_2(
        extract_box(screen, m_text_pokemon),
        qRgb(0, 0, 0), 0.9,
        qRgb(255, 255, 255), 0.1,
        0.2, 50, 0.1
    );
    if (!fight){
        return false;
    }

    fight = false;
    fight |= !fight && cluster_fit_2(
        extract_box(screen, m_text_run),
        qRgb(0, 0, 0), 0.1,
        qRgb(255, 255, 255), 0.9,
        0.2, 50, 0.1
    );
    fight |= !fight && cluster_fit_2(
        extract_box(screen, m_text_run),
        qRgb(0, 0, 0), 0.9,
        qRgb(255, 255, 255), 0.1,
        0.2, 50, 0.1
    );
    if (!fight){
        return false;
    }


    fight = false;
    fight |= !fight && cluster_fit_2(
        extract_box(screen, m_icon_fight),
        qRgb(255, 255, 255), 1.7,
        qRgb(153, 75, 112), 1.0
    );
    fight |= !fight && cluster_fit_2(
        extract_box(screen, m_icon_fight),
        qRgb(0, 0, 0), 1.4,
        qRgb(185, 6, 40), 1.0
    );
    fight |= !fight && cluster_fit_2(   //  Max raid Fight button is a bit different.
        extract_box(screen, m_icon_fight),
        qRgb(0, 0, 0), 1.7,
        qRgb(182, 33, 82), 1.0
    );
//    cout << "===============" << endl;
    if (!fight){
        fight = cluster_fit_2(   //  Cheer
            extract_box(screen, m_icon_fight),
            qRgb(0, 0, 0), 2.2,
            qRgb(9, 162, 218), 1.0
        );
        m_cheer = fight;
    }
    if (!fight){
        return false;
    }

    bool pokemon = false;
    pokemon |= !pokemon && cluster_fit_2(
        extract_box(screen, m_icon_pokemon),
        qRgb(255, 255, 255), 3.1,
        qRgb(126, 224, 142), 1.0
    );
    pokemon |= !pokemon && cluster_fit_2(
        extract_box(screen, m_icon_pokemon),
        qRgb(0, 0, 0), 2.7,
        qRgb(8, 158, 18), 1.0
    );
    if (!pokemon){
        return false;
    }

    bool run = false;
    run |= !run && cluster_fit_2(
        extract_box(screen, m_icon_run),
        qRgb(255, 255, 255), 2.3,
        qRgb(216, 150, 230), 1.0
    );
    run |= !run && cluster_fit_2(
        extract_box(screen, m_icon_run),
        qRgb(0, 0, 0), 1.9,
        qRgb(179, 15, 195), 1.0
    );
    if (!run){
        return false;
    }


    //  Check for white status bar in bottom left corner.
    ImageStats status = image_stats(extract_box(screen, m_status0));
    ImageStats health = image_stats(extract_box(screen, m_status1));
//    cout << status.average << ", " << status.stddev << endl;
    if (is_white(status, 500, 20) && is_white(health)){
        m_dmaxed = false;
        return true;
    }

    //  Check the semi-transparent red status bar if you're dmaxed.
    if (is_solid(status, {0.618001, 0.145809, 0.23619}, 0.15, 40) &&
        is_solid(health, {0.615249, 0.102789, 0.281963}, 0.15, 40)
    ){
        m_dmaxed = true;
        return true;
    }


//    image.save("battle-menu.png");
    return false;
}




BattleMenuReader::BattleMenuReader(VideoOverlay& overlay, Language language)
    : m_language(language)
    , m_opponent_name(overlay, 0.3, 0.010, 0.4, 0.10, COLOR_BLUE)
    , m_summary_opponent_name(overlay, 0.200, 0.100, 0.300, 0.065, COLOR_BLUE)
    , m_summary_opponent_types(overlay, 0.200, 0.170, 0.300, 0.050, COLOR_BLUE)
    , m_own_name(overlay, 0.060, 0.860, 0.160, 0.045, COLOR_BLUE)
    , m_own_sprite(overlay, 0.002, 0.860, 0.060, 0.100, COLOR_BLUE)
    , m_opponent_hp(overlay, 0.360, 0.120, 0.280, 0.005, COLOR_BLUE)
    , m_own_hp(overlay, 0.069, 0.914, 0.204, 0.006, COLOR_BLUE)
    , m_hp0(overlay, 0.073, 0.096 + 0*0.096, 0.052, 0.005, COLOR_BLUE)
    , m_hp1(overlay, 0.073, 0.096 + 1*0.096, 0.052, 0.005, COLOR_BLUE)
    , m_hp2(overlay, 0.073, 0.096 + 2*0.096, 0.052, 0.005, COLOR_BLUE)
    , m_sprite0(overlay, 0.010, 0.040 + 0*0.096, 0.052, 0.061, COLOR_BLUE)
    , m_sprite1(overlay, 0.010, 0.040 + 1*0.096, 0.052, 0.061, COLOR_BLUE)
    , m_sprite2(overlay, 0.010, 0.040 + 2*0.096, 0.052, 0.061, COLOR_BLUE)
    , m_pp0(overlay, 0.902, 0.710 - 1*0.097, 0.070, 0.063, COLOR_BLUE)
    , m_pp1(overlay, 0.902, 0.710 + 0*0.097, 0.070, 0.063, COLOR_BLUE)
    , m_pp2(overlay, 0.902, 0.710 + 1*0.097, 0.070, 0.063, COLOR_BLUE)
    , m_pp3(overlay, 0.902, 0.710 + 2*0.097, 0.070, 0.063, COLOR_BLUE)
    , m_dmax(overlay, 0.541, 0.779, 0.105, 0.186, COLOR_BLUE)
{}

std::set<std::string> BattleMenuReader::read_opponent(
    Logger& logger,
    ProgramEnvironment& env,
    VideoFeed& feed
) const{
    std::set<std::string> result;
    QImage screen;
    for (size_t c = 0; c < 3; c++){
        screen = feed.snapshot();
        QImage image = extract_box(screen, m_opponent_name);
        OCR::TextImageFilter{false, 600}.apply(image);
        result = read_pokemon_name(logger, screen, image, m_language);
        if (!result.empty()){
            return result;
        }
        logger.log("Failed to read opponent name. Retrying in 1 second...", COLOR_ORANGE);
        env.wait_for(std::chrono::seconds(1));
    }
    dump_image(logger, MODULE_NAME, "MaxLair-read_opponent", screen);
    return result;
}
std::set<std::string> BattleMenuReader::read_opponent_in_summary(Logger& logger, const QImage& screen) const{
    QImage name = extract_box(screen, m_summary_opponent_name);
    std::set<std::string> slugs = read_pokemon_name(logger, screen, name, m_language);

    QImage types = extract_box(screen, m_summary_opponent_types);
    std::multimap<double, std::pair<PokemonType, ImagePixelBox>> candidates = find_symbols(types, 0.2);
//    for (const auto& item : candidates){
//        cout << get_type_slug(item.second.first) << ": " << item.first << endl;
//    }

    PokemonType type0 = PokemonType::NONE;
    PokemonType type1 = PokemonType::NONE;
    {
        auto iter = candidates.begin();
        if (iter != candidates.end()){
            type0 = iter->second.first;
            iter++;
        }
        if (iter != candidates.end()){
            type1 = iter->second.first;
            iter++;
        }
    }

    for (auto iter = slugs.begin(); iter != slugs.end();){
        const MaxLairMon& mon = get_maxlair_mon(*iter);
        if ((type0 == mon.type[0] && type1 == mon.type[1]) || (type0 == mon.type[1] && type1 == mon.type[0])){
            ++iter;
        }else{
            iter = slugs.erase(iter);
        }
    }

    if (slugs.size() == 1){
        logger.log("Disambiguation succeeded: " + *slugs.begin(), COLOR_BLUE);
        return slugs;
    }

    if (slugs.empty()){
        logger.log("Disambiguation failed. No results.", COLOR_RED);
    }else{
        logger.log("Disambiguation failed. Still have multiple results: " + set_to_str(slugs), COLOR_RED);
    }

    static std::set<std::string> KNOWN_BAD_SLUGS{
        "basculin-blue-striped",
        "basculin-red-striped",
        "lycanroc-midday",
        "lycanroc-midnight",
    };
    bool error = true;
    for (const std::string& slug : slugs){
        auto iter = KNOWN_BAD_SLUGS.find(slug);
        if (iter != KNOWN_BAD_SLUGS.end()){
            error = false;
            logger.log("Known case that cannot be disambiguated. Skipping error report.", COLOR_RED);
            break;
        }
    }
    if (error){
        dump_image(logger, MODULE_NAME, "DisambiguateBoss", screen);
    }

    return slugs;
}
std::string BattleMenuReader::read_own_mon(Logger& logger, const QImage& screen) const{
    return read_pokemon_name_sprite(
        logger,
        screen,
        m_own_sprite,
        m_own_name, m_language,
        false
    );
}

double BattleMenuReader::read_opponent_hp(Logger& logger, const QImage& screen) const{
    QImage image = extract_box(screen, m_opponent_hp);
//    image.save("test.png");

//    ImageStats stats = image_stats(image);
//    cout << stats.average << " - " << stats.stddev << endl;

#if 0
    double hp = read_hp_bar(image);
    logger.log("Opponent HP: " + (hp < 0 ? "?" : std::to_string(100 * hp)) + "%");
    if (hp < 0){
        dump_image(logger, screen, "BattleMenuReader-read_opponent_hp");
    }
    return hp;
#endif
    return read_hp_bar(logger, image);
}
double BattleMenuReader::read_own_hp(Logger& logger, const QImage& screen) const{
    QImage image = extract_box(screen, m_own_hp);
//    image.save("test.png");
#if 0
    double hp = read_hp_bar(image);
    logger.log("Your HP: " + (hp < 0 ? "?" : std::to_string(100 * hp)) + "%");
    if (hp == 0){
        hp = 0.001;
    }
    if (hp < 0){
        dump_image(logger, screen, "BattleMenuReader-read_own_hp");
    }
#endif
    return read_hp_bar(logger, image);
}
void BattleMenuReader::read_hp(Logger& logger, const QImage& screen, Health health[4], size_t player_index){
    Health tmp_hp[4];
    tmp_hp[0] = {read_own_hp(logger, screen), false};
    tmp_hp[1] = read_in_battle_hp_box(logger, extract_box(screen, m_sprite0), extract_box(screen, m_hp0));
    tmp_hp[2] = read_in_battle_hp_box(logger, extract_box(screen, m_sprite1), extract_box(screen, m_hp1));
    tmp_hp[3] = read_in_battle_hp_box(logger, extract_box(screen, m_sprite2), extract_box(screen, m_hp2));
    bool bad = false;
    for (size_t c = 0; c < 4; c++){
        bad |= tmp_hp[c].hp < 0;
        health[(c + player_index) % 4] = tmp_hp[c];
    }

    if (bad){
        dump_image(logger, MODULE_NAME, "BattlePartyReader-ReadHP", screen);
    }
}
void BattleMenuReader::read_own_pp(Logger& logger, const QImage& screen, int8_t pp[4]) const{
    pp[0] = read_pp_text(logger, extract_box(screen, m_pp0));
    pp[1] = read_pp_text(logger, extract_box(screen, m_pp1));
    pp[2] = read_pp_text(logger, extract_box(screen, m_pp2));
    pp[3] = read_pp_text(logger, extract_box(screen, m_pp3));
    if (pp[0] < 0 && pp[1] < 0 && pp[2] < 0 && pp[3] < 0){
        dump_image(logger, MODULE_NAME, "BattleMenuReader-read_own_pp", screen);
        return;
    }
#if 0
    bool ok = pp[0] > 0 || pp[1] > 0 || pp[2] > 0 || pp[3] > 0;
    if (ok){
        for (size_t c = 0; c < 4; c++){
            pp[c] = std::max(pp[c], (int8_t)0);
        }
    }
#endif
}


bool dmax_circle_ready(QImage image){
    pxint_t w = image.width();
    pxint_t h = image.height();
    if (w * h <= 1){
        return false;
    }

    w = 200;
    h = 200;
    image = image.scaled(w, h);
    if (image_stats(image).stddev.sum() < 75){
        return false;
    }
//    cout << image_stats(image).stddev.sum() << endl;


    size_t total = 0;
    FloatPixel sum;
    FloatPixel sqr_sum;
    for (int r = 0; r < h; r++){
        for (int c = 0; c < w; c++){
            int dy = r - 100;
            int dx = c - 100;
            if (dy < -60){
//                image.setPixelColor(c, r, COLOR_BLUE);
                continue;
            }
            if (-25 < dy && dy < 55){
//                image.setPixelColor(c, r, COLOR_BLUE);
                continue;
            }
            if (dx*dx + dy*dy < 72*72){
//                image.setPixelColor(c, r, COLOR_BLUE);
                continue;
            }
            if (dx*dx + dy*dy > 80*80){
//                image.setPixelColor(c, r, COLOR_BLUE);
                continue;
            }
            FloatPixel p(image.pixel(c, r));
            total++;
            sum += p;
            sqr_sum += p * p;
        }
    }
//    image.save("test.png");

//    size_t total = (size_t)w * (size_t)h;
    FloatPixel variance = (sqr_sum - sum*sum / total) / (total - 1);
    ImageStats stats{
        sum / total,
        FloatPixel(
            std::sqrt(variance.r),
            std::sqrt(variance.g),
            std::sqrt(variance.b)
        )
    };
//    cout << stats.average << stats.stddev << endl;

    if (stats.average.r < 128){
        return false;
    }
    return is_solid(stats, {0.684591, 0.000481775, 0.314928}, 0.1, 50);

//    ImageStats stats = image_stats(image);
}
bool BattleMenuReader::can_dmax(const QImage& screen) const{
    return dmax_circle_ready(extract_box(screen, m_dmax));
}

























}
}
}
}
