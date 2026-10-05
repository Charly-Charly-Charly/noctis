#pragma once

#include <QColor>
#include <QPushButton>

class QEnterEvent;
class QPropertyAnimation;

namespace noctis::ui {

// Botón redondo de ícono (SVG, ver Icons.h) o de texto con fundido animado al pasar el mouse. QSS no
// tiene transiciones, así que el cambio de fondo y color de texto se dibuja a
// mano interpolando según hoverProgress; los colores los fija el stylesheet
// vía qproperty-* (ver Theme.cpp) para que sigan el tema claro/oscuro.
class IconButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(qreal hoverProgress READ hoverProgress WRITE setHoverProgress)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor)
    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)
    Q_PROPERTY(QColor hoverTextColor READ hoverTextColor WRITE setHoverTextColor)

public:
    explicit IconButton(const QString& text, QWidget* parent = nullptr);

    // Nombre de un ícono de ":/icons/" (p. ej. "settings"). Si está fijado se
    // dibuja en lugar del texto, del mismo color que tendría el texto.
    void setIconName(const QString& name);
    const QString& iconName() const { return iconName_; }

    qreal hoverProgress() const { return hoverProgress_; }
    void setHoverProgress(qreal progress);

    QColor borderColor() const { return borderColor_; }
    void setBorderColor(const QColor& color);
    QColor fillColor() const { return fillColor_; }
    void setFillColor(const QColor& color);
    QColor textColor() const { return textColor_; }
    void setTextColor(const QColor& color);
    QColor hoverTextColor() const { return hoverTextColor_; }
    void setHoverTextColor(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void animateHoverTo(qreal target);

    QString iconName_;
    int iconSize_ = 16;
    QPropertyAnimation* hoverAnimation_;
    qreal hoverProgress_ = 0.0;
    QColor borderColor_{0x1D, 0x1D, 0x1B};
    QColor fillColor_{0x1D, 0x1D, 0x1B};
    QColor textColor_{0x1D, 0x1D, 0x1B};
    QColor hoverTextColor_{0xEC, 0xEE, 0xE1};
};

// Botón de ícono listo para usar: `iconName` es un archivo de ":/icons/".
IconButton* makeIconButton(const QString& iconName, QWidget* parent);

} // namespace noctis::ui
