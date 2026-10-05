#pragma once

#include <QDialog>

#include "Core/Ports/ISettingsStore.h"

class QCheckBox;
class QLabel;
class QSpinBox;

namespace noctis::ui {

// Ajustes mínimos pero reales: tema, tabulación y dónde vive todo. No hay un
// formulario general de preferencias todavía porque no hay más preferencias
// que mostrar.
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    SettingsDialog(core::ISettingsStore& settings, bool darkModeEnabled, int tabWidth,
                   const QString& notesRootPath, bool autoCheckUpdates, const QString& appVersion,
                   QWidget* parent = nullptr);

signals:
    void darkModeToggled(bool enabled);
    void tabWidthChanged(int spaces);
    void autoCheckUpdatesToggled(bool enabled);
    void checkUpdatesNowRequested();

    // El cambio recién se aplica al reiniciar Noctis: la raíz de notas se
    // reparte por valor entre MainWindow/SidebarWidget/FolderTreeWidget al
    // arrancar, así que re-plomearla en caliente no vale la pena para v1.
    void notesRootPathChangeRequested(const QString& newPath);

private:
    core::ISettingsStore& settings_;
    QCheckBox* darkModeCheck_;
    QSpinBox* tabWidthSpin_;
    QLabel* notesPathLabel_;
};

} // namespace noctis::ui
