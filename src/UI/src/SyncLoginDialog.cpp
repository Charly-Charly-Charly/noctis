#include "UI/SyncLoginDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QSysInfo>
#include <QVBoxLayout>

namespace noctis::ui {

namespace {
constexpr const char* kServerUrlKey = "sync.serverUrl";
constexpr const char* kEmailKey = "sync.email";
} // namespace

SyncLoginDialog::SyncLoginDialog(core::ISettingsStore& settings, QWidget* parent)
    : QDialog(parent), settings_(settings) {
    setWindowTitle(tr("Iniciar sesión"));

    serverEdit_ = new QLineEdit(this);
    serverEdit_->setPlaceholderText(tr("https://notas.tudominio.com"));

    emailEdit_ = new QLineEdit(this);

    passwordEdit_ = new QLineEdit(this);
    passwordEdit_->setEchoMode(QLineEdit::Password);

    deviceNameEdit_ = new QLineEdit(this);

    if (auto saved = settings_.getString(kServerUrlKey)) {
        serverEdit_->setText(QString::fromStdString(*saved));
    }
    if (auto saved = settings_.getString(kEmailKey)) {
        emailEdit_->setText(QString::fromStdString(*saved));
    }
    deviceNameEdit_->setText(QSysInfo::machineHostName());

    auto* form = new QFormLayout();
    form->addRow(tr("Servidor"), serverEdit_);
    form->addRow(tr("Correo"), emailEdit_);
    form->addRow(tr("Contraseña"), passwordEdit_);
    form->addRow(tr("Este dispositivo"), deviceNameEdit_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(buttons);
}

std::string SyncLoginDialog::serverUrl() const {
    return serverEdit_->text().trimmed().toStdString();
}

std::string SyncLoginDialog::email() const {
    return emailEdit_->text().trimmed().toStdString();
}

std::string SyncLoginDialog::password() const {
    return passwordEdit_->text().toStdString();
}

std::string SyncLoginDialog::deviceName() const {
    QString name = deviceNameEdit_->text().trimmed();
    return (name.isEmpty() ? tr("Dispositivo") : name).toStdString();
}

void SyncLoginDialog::accept() {
    if (serverEdit_->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, tr("Falta el servidor"),
                              tr("Escribe la dirección de tu servidor, p. ej.\n"
                                 "https://notas.tudominio.com"));
        return;
    }
    if (emailEdit_->text().trimmed().isEmpty() || passwordEdit_->text().isEmpty()) {
        QMessageBox::warning(this, tr("Faltan datos"), tr("Completa tu correo y contraseña."));
        return;
    }

    QDialog::accept();
}

} // namespace noctis::ui
