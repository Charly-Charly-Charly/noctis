#pragma once

#include <QDialog>

#include <string>

#include "Core/Ports/ISettingsStore.h"

class QLineEdit;

namespace noctis::ui {

// Formulario para configurar el servidor de sincronización e iniciar sesión.
// Recuerda el servidor y el email entre sesiones; la contraseña nunca se
// persiste aquí (SyncService guarda el token, no la contraseña).
class SyncLoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit SyncLoginDialog(core::ISettingsStore& settings, QWidget* parent = nullptr);

    std::string serverUrl() const;
    std::string email() const;
    std::string password() const;
    std::string deviceName() const;

    void accept() override;

private:
    core::ISettingsStore& settings_;

    QLineEdit* serverEdit_;
    QLineEdit* emailEdit_;
    QLineEdit* passwordEdit_;
    QLineEdit* deviceNameEdit_;
};

} // namespace noctis::ui
