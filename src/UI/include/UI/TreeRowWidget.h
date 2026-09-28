#pragma once

#include <QWidget>

class QLabel;

namespace noctis::ui {

// Una fila del árbol: [número][icono][título][espacio][punto de "sin
// guardar"]. Se usa vía QTreeWidget::setItemWidget en vez del texto plano
// del item, así que el resaltado de "nota activa" no es la selección nativa
// de Qt (que un click en el árbol puede perder) sino un estado propio que
// MainWindow controla explícitamente con setActive().
class TreeRowWidget : public QWidget {
    Q_OBJECT

public:
    enum class Kind { Section, Folder, Note };

    TreeRowWidget(const QString& number, const QString& icon, const QString& title, Kind kind,
                  QWidget* parent = nullptr);

    void setActive(bool active);
    void setDirty(bool dirty);

signals:
    void activated();
    void contextMenuRequested(const QPoint& globalPos);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QLabel* dotLabel_;
};

} // namespace noctis::ui
