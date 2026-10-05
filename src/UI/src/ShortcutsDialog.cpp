#include "UI/ShortcutsDialog.h"

#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace noctis::ui {

namespace {
struct Shortcut {
    const char* keys;
    const char* action;
};

constexpr Shortcut kShortcuts[] = {
    {"Ctrl+N", "Nueva nota"},
    {"Ctrl+S", "Guardar"},
    {"Ctrl+W", "Cerrar pestaña"},
    {"Ctrl+Shift+T", "Reabrir pestaña cerrada"},
    {"Ctrl+F", "Buscar en la nota"},
    {"Ctrl+K", "Buscar en todas las notas"},
    {"Ctrl+Shift+S", "Exportar"},
    {"Alt+1", "Vista: Editor"},
    {"Alt+2", "Vista: Vista previa"},
    {"Alt+3", "Vista: Split"},
    {"Alt+Z", "Ajuste de línea"},
    {"Ctrl++ / Ctrl+- / Ctrl+0", "Acercar / Alejar / Restablecer zoom"},
    {"Ctrl+rueda", "Zoom"},
    {"Ctrl+B / Ctrl+I", "Negrita / Cursiva"},
    {"Ctrl+Shift+X / Ctrl+U", "Tachado / Subrayado"},
    {"Alt+clic", "Añadir o quitar un cursor"},
    {"Esc", "Volver a un solo cursor"},
    {"Tab / Shift+Tab", "Indentar / quitar sangría (varias líneas)"},
    {"Ctrl+Shift+D", "Modo oscuro"},
    {"F5", "Sincronizar ahora"},
};
} // namespace

ShortcutsDialog::ShortcutsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Atajos de teclado"));

    auto* form = new QFormLayout();
    for (const Shortcut& shortcut : kShortcuts) {
        auto* keysLabel = new QLabel(QString::fromUtf8(shortcut.keys), this);
        keysLabel->setObjectName("shortcutKeys");
        form->addRow(keysLabel, new QLabel(tr(shortcut.action), this));
    }

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(form);
}

} // namespace noctis::ui
