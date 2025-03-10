/*  Switch Date
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <QJsonValue>
#include "Common/Qt/Options/SwitchDate/SwitchDateBaseWidget.h"
#include "SwitchDateOption.h"

namespace PokemonAutomation{
namespace NintendoSwitch{


class SwitchDateWidget : private SwitchDateBaseWidget, public ConfigWidget{
public:
    SwitchDateWidget(QWidget& parent, SwitchDateOption& value)
        : SwitchDateBaseWidget(parent, value)
        , ConfigWidget(value, *this)
    {}
    virtual void restore_defaults() override{
        SwitchDateBaseWidget::restore_defaults();
    }
};



SwitchDateOption::SwitchDateOption(
    QString label,
    QDate default_value
)
    : SwitchDateBaseOption(std::move(label), default_value)
{}
void SwitchDateOption::load_json(const QJsonValue& json){
    return this->load_current(json);
}
QJsonValue SwitchDateOption::to_json() const{
    return this->write_current();
}

QString SwitchDateOption::check_validity() const{
    return SwitchDateBaseOption::check_validity();
}
void SwitchDateOption::restore_defaults(){
    SwitchDateBaseOption::restore_defaults();
}


ConfigWidget* SwitchDateOption::make_ui(QWidget& parent){
    return new SwitchDateWidget(parent, *this);
}



}
}
