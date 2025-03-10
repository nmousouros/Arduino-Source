/*  Config Option
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_ConfigOption_H
#define PokemonAutomation_ConfigOption_H

class QString;
class QJsonValue;
class QWidget;

namespace PokemonAutomation{


enum class ConfigOptionState{
    ENABLED,
    DISABLED,
    HIDDEN,
};


class ConfigWidget;

class ConfigOption{
public:
    virtual ~ConfigOption() = default;

    virtual void load_json(const QJsonValue& json) = 0;
    virtual QJsonValue to_json() const = 0;

    //  Returns error message if invalid. Otherwise returns empty string.
    virtual QString check_validity() const;

    virtual void restore_defaults(){};

    //  This is called by the framework at the start of a program to reset any
    //  transient state that the option object may have.
    virtual void reset_state(){};

    virtual ConfigWidget* make_ui(QWidget& parent) = 0;

public:
    ConfigOptionState visibility = ConfigOptionState::ENABLED;
};


class ConfigWidget{
public:
    virtual ~ConfigWidget() = default;
    ConfigWidget(ConfigOption& m_value, QWidget& widget);

    const ConfigOption& option() const{ return m_value; }
    ConfigOption& option(){ return m_value; }

    QWidget& widget(){ return m_widget; }
    virtual void restore_defaults() = 0;

    virtual void update_visibility();

private:
    ConfigOption& m_value;
    QWidget& m_widget;
};


}
#endif
