#pragma once

#include <QColor>
#include <QWidget>

class QEnterEvent;
class QLabel;
class QPropertyAnimation;

namespace noctis::ui {

// Una fila del árbol: [número][icono][título][espacio][punto de "sin
// guardar"]. Se usa vía QTreeWidget::setItemWidget en vez del texto plano
// del item, así que el resaltado de "nota activa" no es la selección nativa
// de Qt (que un click en el árbol puede perder) sino un estado propio que
// MainWindow controla explícitamente con setActive().
class TreeRowWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)
    Q_PROPERTY(QColor hoverColor READ hoverColor WRITE setHoverColor)

public:
    enum class Kind { Section, Folder, Note };

    TreeRowWidget(const QString& number, const QString& icon, const QString& title, Kind kind,
                  QWidget* parent = nullptr);

    void setActive(bool active);
    void setDirty(bool dirty);

    qreal hoverProgress() const { return hoverProgress_; }
    void setHoverProgress(qreal progress);

    // Lo fija el stylesheet (qproperty-hoverColor) para seguir el tema.
    QColor hoverColor() const { return hoverColor_; }
    void setHoverColor(const QColor& color);

signals:
    void activated();
    void contextMenuRequested(const QPoint& globalPos);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void animateHoverTo(qreal target);

    QLabel* dotLabel_;
    QPropertyAnimation* hoverAnimation_;
    qreal hoverProgress_ = 0.0;
    QColor hoverColor_{0xF2, 0xF3, 0xE9};
};

} // namespace noctis::ui
