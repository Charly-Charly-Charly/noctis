#pragma once

#include <QDialog>

namespace noctis::ui {

// "Acerca de" mínimo: qué es Noctis y su filosofía local-first. No hay
// changelog ni versión dinámica todavía porque el proyecto no los tiene.
class HelpDialog : public QDialog {
    Q_OBJECT

public:
    explicit HelpDialog(QWidget* parent = nullptr);
};

} // namespace noctis::ui
