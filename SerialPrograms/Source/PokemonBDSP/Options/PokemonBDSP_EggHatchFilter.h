/*  Egg Hatch Filter
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonBDSP_EggHatchFilter_H
#define PokemonAutomation_PokemonBDSP_EggHatchFilter_H

#include "CommonFramework/Options/ConfigOption.h"
#include "CommonFramework/Options/EditableTableOption.h"
#include "Pokemon/Pokemon_IVChecker.h"
#include "Pokemon/Inference/Pokemon_IVCheckerReader.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{
using namespace Pokemon;


enum class EggHatchAction{
    StopProgram,
    Keep,
    Release,
};

enum class EggHatchShinyFilter{
    Anything,
    NotShiny,
    Shiny,
};


class EggHatchFilterRow : public EditableTableRow{
public:
    EggHatchFilterRow(EggHatchShinyFilter p_shiny = EggHatchShinyFilter::Anything);
    virtual void load_json(const QJsonValue& json) override;
    virtual QJsonValue to_json() const override;
    virtual std::unique_ptr<EditableTableRow> clone() const override;
    virtual std::vector<QWidget*> make_widgets(QWidget& parent) override;

private:
    QWidget* make_action_box(QWidget& parent);
    QWidget* make_shiny_box(QWidget& parent);
    QWidget* make_iv_box(QWidget& parent, IVCheckerFilter& iv);

public:
    EggHatchAction action = EggHatchAction::Keep;
    EggHatchShinyFilter shiny = EggHatchShinyFilter::Anything;
    IVCheckerFilter iv_hp = IVCheckerFilter::Anything;
    IVCheckerFilter iv_atk = IVCheckerFilter::Anything;
    IVCheckerFilter iv_def = IVCheckerFilter::Anything;
    IVCheckerFilter iv_spatk = IVCheckerFilter::Anything;
    IVCheckerFilter iv_spdef = IVCheckerFilter::Anything;
    IVCheckerFilter iv_speed = IVCheckerFilter::Anything;
};

class EggHatchFilterOptionFactory : public EditableTableFactory{
public:
    virtual QStringList make_header() const override;
    virtual std::unique_ptr<EditableTableRow> make_row() const override;
};


class EggHatchFilterOption : public ConfigOption{
public:
    EggHatchFilterOption();

    EggHatchAction get_action(bool shiny, const IVCheckerReader::Results& IVs) const;

    virtual void load_json(const QJsonValue& json) override;
    virtual QJsonValue to_json() const override;

    virtual void restore_defaults() override;

    virtual ConfigWidget* make_ui(QWidget& parent) override;

private:
    std::vector<std::unique_ptr<EditableTableRow>> make_defaults() const;

private:
    EggHatchFilterOptionFactory m_factory;
    EditableTableOption m_table;
};





}
}
}
#endif
