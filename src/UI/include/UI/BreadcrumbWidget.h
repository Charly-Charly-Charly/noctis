#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QGraphicsOpacityEffect;
class QDateTime;

namespace noctis::ui {

// Fila bajo las pestañas: volver, ruta de la nota activa y fecha/hora del
// último guardado (con un pulso de opacidad real, no cosmético), más un menú
// de más opciones. El estado "Editando…/Guardando…/Guardado" en sí vive en la
// barra de estado, junto al contador de caracteres — no se duplica aquí.
class BreadcrumbWidget : public QWidget {
    Q_OBJECT

public:
    explicit BreadcrumbWidget(QWidget* parent = nullptr);

    void setPath(const QString& path);
    void showSavedPulse(const QDateTime& savedAt); // fecha/hora + pulso
    void setEmpty();                        // no hay nota abierta

signals:
    void backRequested();
    void moreOptionsRequested(const QPoint& globalPos);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QLabel* pathLabel_;
    QLabel* dateLabel_;
    QGraphicsOpacityEffect* dateOpacity_;
    QPushButton* backButton_;
    QPushButton* moreButton_;
};

} // namespace noctis::ui
