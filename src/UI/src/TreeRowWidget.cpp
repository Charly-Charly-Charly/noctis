#include "UI/TreeRowWidget.h"

#include <QContextMenuEvent>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyle>

namespace noctis::ui {

namespace {
constexpr int kHoverDurationMs = 140;
constexpr qreal kRowRadius = 8.0;
} // namespace

TreeRowWidget::TreeRowWidget(const QString& number, const QString& icon, const QString& title,
                              Kind kind, QWidget* parent)
    : QWidget(parent) {
    // Selector por id y no por nombre de clase: Qt compara el nombre
    // calificado ("noctis::ui::TreeRowWidget"), así que un selector
    // "TreeRowWidget" a secas no es confiable dentro de un namespace.
    setObjectName("treeRow");

    // Sin esto, el "background" del QSS para #treeRow[active="true"]
    // simplemente no se pinta: los QWidget personalizados no aplican el
    // fondo del stylesheet por defecto.
    setAttribute(Qt::WA_StyledBackground, true);
    setProperty("active", false);
    setCursor(Qt::PointingHandCursor);

    hoverAnimation_ = new QPropertyAnimation(this, "hoverProgress", this);
    hoverAnimation_->setDuration(kHoverDurationMs);
    hoverAnimation_->setEasingCurve(QEasingCurve::OutCubic);

    auto* numberLabel = new QLabel(number, this);
    numberLabel->setObjectName("treeRowNumber");
    numberLabel->setFixedWidth(28);

    auto* titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("treeRowTitle");
    if (kind == Kind::Section) titleLabel->setProperty("section", true);

    dotLabel_ = new QLabel(QString::fromUtf8("●"), this); // ●
    dotLabel_->setObjectName("treeRowDot");
    dotLabel_->setFixedWidth(12);
    dotLabel_->setVisible(false);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 8, 2);
    layout->setSpacing(6);
    layout->addWidget(numberLabel);
    if (!icon.isEmpty()) {
        auto* iconLabel = new QLabel(icon, this);
        iconLabel->setObjectName("treeRowIcon");
        iconLabel->setFixedWidth(18);
        layout->addWidget(iconLabel);
    }
    layout->addWidget(titleLabel, 1);
    layout->addWidget(dotLabel_);
}

void TreeRowWidget::setActive(bool active) {
    setProperty("active", active);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void TreeRowWidget::setDirty(bool dirty) {
    dotLabel_->setVisible(dirty);
}

void TreeRowWidget::setHoverProgress(qreal progress) {
    hoverProgress_ = progress;
    update();
}

void TreeRowWidget::setHoverColor(const QColor& color) {
    hoverColor_ = color;
    update();
}

void TreeRowWidget::enterEvent(QEnterEvent* event) {
    animateHoverTo(1.0);
    QWidget::enterEvent(event);
}

void TreeRowWidget::leaveEvent(QEvent* event) {
    animateHoverTo(0.0);
    QWidget::leaveEvent(event);
}

void TreeRowWidget::animateHoverTo(qreal target) {
    hoverAnimation_->stop();
    hoverAnimation_->setStartValue(hoverProgress_);
    hoverAnimation_->setEndValue(target);
    hoverAnimation_->start();
}

void TreeRowWidget::paintEvent(QPaintEvent* event) {
    // El fondo de QSS (fila activa) ya se pintó antes de llegar acá; el
    // resaltado de hover va encima y debajo de los labels, que son
    // transparentes. La fila activa ya tiene su propio color: no se tiñe.
    QWidget::paintEvent(event);
    if (hoverProgress_ <= 0.0 || property("active").toBool()) return;

    QColor tint = hoverColor_;
    tint.setAlphaF(tint.alphaF() * hoverProgress_);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(tint);
    painter.drawRoundedRect(rect(), kRowRadius, kRowRadius);
}

void TreeRowWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) emit activated();
    QWidget::mousePressEvent(event);
}

void TreeRowWidget::contextMenuEvent(QContextMenuEvent* event) {
    emit contextMenuRequested(event->globalPos());
}

} // namespace noctis::ui
