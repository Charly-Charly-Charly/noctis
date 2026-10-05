#pragma once

#include <QObject>

class QListWidget;
class QPropertyAnimation;
class QPushButton;

namespace noctis::ui {

// Fundido al pasar el mouse para un QPushButton estilado por QSS (que no
// tiene transiciones). Pinta el botón dos veces: en su estado normal y encima
// en estado hover con opacidad creciente, así reutiliza las reglas :hover y
// :pressed del stylesheet en vez de duplicar colores acá.
class ButtonHoverFade : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal progress READ progress WRITE setProgress)

public:
    explicit ButtonHoverFade(QPushButton* button);

    qreal progress() const { return progress_; }
    void setProgress(qreal progress);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    bool paint();
    void animateTo(qreal target);

    QPushButton* button_;
    QPropertyAnimation* animation_;
    qreal progress_ = 0.0;
};

// Filtro de aplicación: le agrega ButtonHoverFade a cada QPushButton la
// primera vez que se muestra (incluye los que crea QDialogButtonBox o
// QMessageBox por su cuenta). Los IconButton se dibujan solos y se saltean.
class HoverFadeInstaller : public QObject {
    Q_OBJECT

public:
    using QObject::QObject;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};

// Hover con fundido por fila en una QListWidget. El color sale de la
// propiedad alternate-background-color del stylesheet (ver Theme.cpp), para
// seguir el tema claro/oscuro.
void installListHoverFade(QListWidget* list);

} // namespace noctis::ui
