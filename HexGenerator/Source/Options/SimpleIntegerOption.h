/*  Simple Integer
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_SimpleInteger_H
#define PokemonAutomation_SimpleInteger_H

#include "Common/Qt/Options/SimpleInteger/SimpleIntegerBaseOption.h"
#include "Common/Qt/Options/SimpleInteger/SimpleIntegerBaseWidget.h"
#include "SingleStatementOption.h"

namespace PokemonAutomation{


class SimpleInteger : public SingleStatementOption, private SimpleIntegerBaseOption<uint32_t>{
public:
    static const QString OPTION_TYPE;
    static const QString JSON_MIN_VALUE;
    static const QString JSON_MAX_VALUE;

public:
    SimpleInteger(const QJsonObject& obj);

    virtual const QString& type() const override{ return OPTION_TYPE; }
    virtual QString check_validity() const override;
    virtual void restore_defaults() override;

    virtual QJsonObject to_json() const override;
    virtual std::string to_cpp() const override;

    virtual QWidget* make_ui(QWidget& parent) override;

private:
    friend class SimpleIntegerUI;
};

class SimpleIntegerUI : public SimpleIntegerBaseWidget<uint32_t>{
public:
    SimpleIntegerUI(QWidget& parent, SimpleInteger& value);
};


}
#endif


