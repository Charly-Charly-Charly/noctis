#include "UI/TreeRowWidget.h"

#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QStyle>

namespace noctis::ui {

TreeRowWidget::TreeRowWidget(const QString& number, const QString& icon, const QString& title,
                              Kind kind, QWidget* parent)
    : QWidget(parent) {
    // Sin esto, el "background" del QSS para TreeRowWidget[active="true"]
    // simplemente no se pinta: los QWidget personalizados no aplican el
    // fondo del stylesheet por defecto.
    setAttribute(Qt::WA_StyledBackground, true);
    setProperty("active", false);

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

void TreeRowWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) emit activated();
    QWidget::mousePressEvent(event);
}

void TreeRowWidget::contextMenuEvent(QContextMenuEvent* event) {
    emit contextMenuRequested(event->globalPos());
}

} // namespace noctis::ui
