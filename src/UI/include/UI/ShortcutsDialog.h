#pragma once

#include <QDialog>

namespace noctis::ui {

// Lista estática de los atajos ya registrados en MainWindow::setupMenus().
// Si se agrega un atajo nuevo ahí, hay que reflejarlo aquí también: no hay
// forma de leerlos dinámicamente sin acoplar este diálogo al menú.
class ShortcutsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ShortcutsDialog(QWidget* parent = nullptr);
};

} // namespace noctis::ui
