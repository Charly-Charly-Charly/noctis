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
    {"Ctrl+K / Ctrl+F", "Buscar"},
    {"Alt+1", "Vista: Editor"},
    {"Alt+2", "Vista: Vista previa"},
    {"Alt+3", "Vista: Split"},
    {"Alt+Z", "Ajuste de línea"},
    {"Ctrl+Shift+D", "Modo oscuro"},
    {"Ctrl+Shift+S", "Sincronizar ahora"},
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
