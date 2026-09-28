#include "UI/InsertTagDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace noctis::ui {

InsertTagDialog::InsertTagDialog(const std::vector<std::string>& existingTags, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Insertar etiqueta"));
    setMinimumWidth(280);

    tagCombo_ = new QComboBox(this);
    tagCombo_->setEditable(true);
    tagCombo_->setInsertPolicy(QComboBox::NoInsert);
    for (const std::string& tag : existingTags) {
        tagCombo_->addItem(QString::fromStdString(tag));
    }
    tagCombo_->setCurrentText(QString());

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Etiqueta (nueva o existente):"), this));
    layout->addWidget(tagCombo_);
    layout->addWidget(buttons);
}

QString InsertTagDialog::tag() const {
    QString text = tagCombo_->currentText().trimmed();
    if (text.startsWith('#')) text = text.mid(1);
    return text.replace(QRegularExpression("\\s+"), "-");
}

} // namespace noctis::ui
