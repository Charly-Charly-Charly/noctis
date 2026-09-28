#pragma once

#include <QWidget>

#include "Core/Ports/INoteRepository.h"
#include "Core/Services/RecentsService.h"

class QListWidget;
class QLabel;

namespace noctis::ui {

// Pantalla que ocupa el área de contenido cuando no hay ninguna nota
// abierta: crear una nueva o retomar una reciente. Reemplaza el antiguo
// botón "+" suelto junto a las pestañas.
class WelcomeScreen : public QWidget {
    Q_OBJECT

public:
    WelcomeScreen(core::INoteRepository& repository, core::RecentsService& recentsService,
                  QWidget* parent = nullptr);

    void refresh(); // relee las notas recientes

signals:
    void newNoteRequested();
    void noteSelected(const core::NoteId& id);

private:
    core::INoteRepository& repository_;
    core::RecentsService& recentsService_;

    QListWidget* recentsList_;
    QLabel* emptyHint_;
};

} // namespace noctis::ui
