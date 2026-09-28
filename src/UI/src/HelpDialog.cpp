#include "UI/HelpDialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

namespace noctis::ui {

HelpDialog::HelpDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Acerca de Noctis"));
    setMinimumWidth(360);

    auto* titleLabel = new QLabel(tr("Noctis"), this);
    titleLabel->setObjectName("helpTitle");

    auto* bodyLabel = new QLabel(
        tr("Editor de notas Markdown local-first: tus notas son archivos .md reales en "
           "tu disco, no una base de datos propietaria.\n\n"
           "La sincronización con otros dispositivos es opcional (menú Cuenta): el "
           "servidor solo guarda una réplica y tus anotaciones, nunca reemplaza al "
           "archivo local."),
        this);
    bodyLabel->setWordWrap(true);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
    layout->addWidget(bodyLabel);
    layout->addWidget(buttons);
}

} // namespace noctis::ui
