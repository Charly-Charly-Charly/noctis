#include "UI/SettingsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace noctis::ui {

SettingsDialog::SettingsDialog(core::ISettingsStore& settings, bool darkModeEnabled, int tabWidth,
                                const QString& notesRootPath, QWidget* parent)
    : QDialog(parent), settings_(settings) {
    setWindowTitle(tr("Ajustes"));
    setMinimumWidth(380);

    darkModeCheck_ = new QCheckBox(tr("Modo oscuro"), this);
    darkModeCheck_->setChecked(darkModeEnabled);
    connect(darkModeCheck_, &QCheckBox::toggled, this, &SettingsDialog::darkModeToggled);

    tabWidthSpin_ = new QSpinBox(this);
    tabWidthSpin_->setRange(2, 8);
    tabWidthSpin_->setValue(tabWidth);
    connect(tabWidthSpin_, &QSpinBox::valueChanged, this, &SettingsDialog::tabWidthChanged);

    notesPathLabel_ = new QLabel(notesRootPath, this);
    notesPathLabel_->setWordWrap(true);
    notesPathLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* changeFolderButton = new QPushButton(tr("Cambiar carpeta…"), this);
    changeFolderButton->setToolTip(tr("Requiere reiniciar Noctis para terminar de aplicarse."));
    connect(changeFolderButton, &QPushButton::clicked, this, [this] {
        QString chosen = QFileDialog::getExistingDirectory(this, tr("Carpeta de notas"),
                                                             notesPathLabel_->text());
        if (chosen.isEmpty()) return;

        notesPathLabel_->setText(chosen);
        emit notesRootPathChangeRequested(chosen);
    });

    auto* notesPathRow = new QWidget(this);
    auto* notesPathLayout = new QHBoxLayout(notesPathRow);
    notesPathLayout->setContentsMargins(0, 0, 0, 0);
    notesPathLayout->addWidget(notesPathLabel_, 1);
    notesPathLayout->addWidget(changeFolderButton);

    QString serverUrl = tr("(sin configurar)");
    if (auto stored = settings_.getString("sync.serverUrl"); stored && !stored->empty()) {
        serverUrl = QString::fromStdString(*stored);
    }
    auto* serverLabel = new QLabel(serverUrl, this);
    serverLabel->setWordWrap(true);
    serverLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* form = new QFormLayout();
    form->addRow(tr("Apariencia"), darkModeCheck_);
    form->addRow(tr("Tabulación (espacios)"), tabWidthSpin_);
    form->addRow(tr("Carpeta de notas"), notesPathRow);
    form->addRow(tr("Servidor de sincronización"), serverLabel);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

} // namespace noctis::ui
