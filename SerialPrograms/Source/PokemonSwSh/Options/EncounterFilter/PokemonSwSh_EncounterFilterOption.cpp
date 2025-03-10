/*  Encounter Filter
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Qt/QtJsonTools.h"
#include "CommonFramework/Globals.h"
#include "PokemonSwSh_EncounterFilterEnums.h"
#include "PokemonSwSh_EncounterFilterOption.h"
#include "PokemonSwSh_EncounterFilterWidget.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{


class EncounterFilterOptionFactory : public EditableTableFactory{
public:
    EncounterFilterOptionFactory(bool rare_stars);
    virtual QStringList make_header() const override;
    virtual std::unique_ptr<EditableTableRow> make_row() const override;
private:
    bool m_rare_stars;
};

EncounterFilterOptionFactory::EncounterFilterOptionFactory(bool rare_stars)
    : m_rare_stars(rare_stars)
{}
QStringList EncounterFilterOptionFactory::make_header() const{
    QStringList list;
    list << "Action" << STRING_POKEBALL << STRING_POKEMON << "Shininess";
    return list;
}
std::unique_ptr<EditableTableRow> EncounterFilterOptionFactory::make_row() const{
    return std::unique_ptr<EditableTableRow>(new EncounterFilterOverride(m_rare_stars));
}

const EncounterFilterOptionFactory ENCOUNTER_FILTER_NORMAL(false);
const EncounterFilterOptionFactory ENCOUNTER_FILTER_RARE(true);





EncounterFilterOption::~EncounterFilterOption(){
}
EncounterFilterOption::EncounterFilterOption(bool rare_stars, bool enable_overrides)
    : m_label("<b>Encounter Filter:</b>")
    , m_rare_stars(rare_stars)
    , m_enable_overrides(enable_overrides)
    , m_shiny_filter_default(ShinyFilter::ANY_SHINY)
    , m_shiny_filter_current(m_shiny_filter_default)
    , m_table(
        rare_stars
            ?   "<b>Overrides:</b><br>"
                "The game language must be properly set to read " + STRING_POKEMON + " names. "
                "If more than one override applies, the last one will be chosen.<br>"
                "<font color=\"red\">Due to the extreme rarity of star shinies (1 in 6 million), "
                "the filters here will not allow you to run from them. "
                "If you get a star shiny, catch it and cherish it.</font>"
            :
                "<b>Overrides:</b><br>"
                "The game language must be properly set to read " + STRING_POKEMON + " names.<br>"
                "If more than one override applies, the last one will be chosen.",
        rare_stars ? ENCOUNTER_FILTER_RARE : ENCOUNTER_FILTER_NORMAL,
        false
    )
{}
std::vector<EncounterFilterOverride> EncounterFilterOption::overrides() const{
    std::vector<EncounterFilterOverride> ret;
    for (size_t c = 0; c < m_table.size(); c++){
        ret.emplace_back(static_cast<const EncounterFilterOverride&>(m_table[c]));
    }
    return ret;
}
void EncounterFilterOption::load_json(const QJsonValue& json){
    using namespace Pokemon;

    QJsonObject obj = json.toObject();

    QString shiny_filter;
    if (json_get_string(shiny_filter, obj, "ShinyFilter")){
        auto iter = ShinyFilter_MAP.find(shiny_filter);
        if (iter != ShinyFilter_MAP.end()){
            m_shiny_filter_current = iter->second;
        }
    }

    if (m_enable_overrides){
        m_table.load_json(json_get_array_nothrow(obj, "Overrides"));
    }
}
QJsonValue EncounterFilterOption::to_json() const{
    QJsonObject obj;
    obj.insert("ShinyFilter", ShinyFilter_NAMES[(size_t)m_shiny_filter_current]);

    if (m_enable_overrides){
        obj.insert("Overrides", m_table.to_json());
    }

    return obj;
}
void EncounterFilterOption::restore_defaults(){
    m_shiny_filter_current = m_shiny_filter_default;
    m_table.restore_defaults();
}
ConfigWidget* EncounterFilterOption::make_ui(QWidget& parent){
    return new EncounterFilterWidget(parent, *this);
}




}
}
}
