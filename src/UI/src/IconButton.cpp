#include "UI/IconButton.h"

#include <QEasingCurve>
#include <QEnterEvent>
#include <QPainter>
#include <QPropertyAnimation>

namespace noctis::ui {

namespace {
constexpr int kHoverDurationMs = 140;

QColor mix(const QColor& from, const QColor& to, qreal t) {
    return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * t,
                            from.greenF() + (to.greenF() - from.greenF()) * t,
                            from.blueF() + (to.blueF() - from.blueF()) * t,
                            from.alphaF() + (to.alphaF() - from.alphaF()) * t);
}
} // namespace

IconButton::IconButton(const QString& text, QWidget* parent) : QPushButton(text, parent) {
    setCursor(Qt::PointingHandCursor);

    hoverAnimation_ = new QPropertyAnimation(this, "hoverProgress", this);
    hoverAnimation_->setDuration(kHoverDurationMs);
    hoverAnimation_->setEasingCurve(QEasingCurve::OutCubic);
}

void IconButton::setHoverProgress(qreal progress) {
    hoverProgress_ = progress;
    update();
}

void IconButton::setBorderColor(const QColor& color) {
    borderColor_ = color;
    update();
}

void IconButton::setFillColor(const QColor& color) {
    fillColor_ = color;
    update();
}

void IconButton::setTextColor(const QColor& color) {
    textColor_ = color;
    update();
}

void IconButton::setHoverTextColor(const QColor& color) {
    hoverTextColor_ = color;
    update();
}

void IconButton::enterEvent(QEnterEvent* event) {
    animateHoverTo(1.0);
    QPushButton::enterEvent(event);
}

void IconButton::leaveEvent(QEvent* event) {
    animateHoverTo(0.0);
    QPushButton::leaveEvent(event);
}

void IconButton::animateHoverTo(qreal target) {
    hoverAnimation_->stop();
    hoverAnimation_->setStartValue(hoverProgress_);
    hoverAnimation_->setEndValue(target);
    hoverAnimation_->start();
}

void IconButton::paintEvent(QPaintEvent*) {
    // Apretado se ve igual que con el mouse encima, sin esperar al fundido.
    const qreal t = isDown() ? 1.0 : hoverProgress_;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const QRectF bounds = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    const qreal radius = qMin(bounds.width(), bounds.height()) / 2.0;

    QColor fill = fillColor_;
    fill.setAlphaF(fill.alphaF() * t);
    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);
    painter.drawRoundedRect(bounds, radius, radius);

    painter.setPen(QPen(borderColor_, 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(bounds, radius, radius);

    painter.setPen(mix(textColor_, hoverTextColor_, t));
    painter.setFont(font());
    painter.drawText(rect(), Qt::AlignCenter, text());
}

} // namespace noctis::ui
