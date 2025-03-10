/*  String Option
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <QHBoxLayout>
#include <QLabel>
#include "StringBaseWidget.h"

namespace PokemonAutomation{



StringBaseWidget::StringBaseWidget(QWidget& parent, StringBaseOption& value)
    : QWidget(&parent)
    , m_value(value)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    QLabel* text = new QLabel(value.label(), this);
    layout->addWidget(text, 1);
    text->setWordWrap(true);

    m_line_edit = new QLineEdit(m_value.get());
    m_line_edit->setPlaceholderText(value.placeholder_text());
    layout->addWidget(m_line_edit, 1);

    if (m_value.is_password()){
        m_line_edit->setEchoMode(QLineEdit::PasswordEchoOnEdit);
    }

    connect(
        m_line_edit, &QLineEdit::editingFinished,
        this, [=](){
            m_value.set(m_line_edit->text());
        }
    );
}
void StringBaseWidget::restore_defaults(){
    m_value.restore_defaults();
    m_line_edit->setText(m_value.get());
}



}
