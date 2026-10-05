#pragma once

#include <QColor>
#include <QPixmap>
#include <QString>
#include <QWidget>

namespace noctis::ui {

namespace icons {

// Pixmap del ícono `name` (recurso ":/icons/<name>.svg", Material Icons de
// Google; ver resources/icons/LICENSE.txt) repintado de `color`. Los SVG son de
// un solo color, así que el tema claro/oscuro no necesita archivos distintos.
// `size` en píxeles lógicos; `devicePixelRatio` mantiene el trazo nítido en
// pantallas HiDPI.
QPixmap pixmap(const QString& name, const QColor& color, int size, qreal devicePixelRatio = 1.0);

} // namespace icons

// Ícono suelto (no clicable) que toma su color del `color:` del stylesheet,
// igual que lo haría un QLabel de texto: así sigue solo el tema y los estados
// (p. ej. la fila activa del árbol invierte su color).
class IconLabel : public QWidget {
    Q_OBJECT

public:
    IconLabel(const QString& iconName, int iconSize, QWidget* parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    QString iconName_;
    int iconSize_;
};

} // namespace noctis::ui
