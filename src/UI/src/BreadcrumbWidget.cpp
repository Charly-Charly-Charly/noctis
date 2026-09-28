#include "UI/BreadcrumbWidget.h"

#include <QContextMenuEvent>
#include <QDateTime>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>

namespace noctis::ui {

BreadcrumbWidget::BreadcrumbWidget(QWidget* parent) : QWidget(parent) {
    setObjectName("breadcrumb");
    setAttribute(Qt::WA_StyledBackground, true);

    backButton_ = new QPushButton(QString::fromUtf8("←"), this);
    backButton_->setObjectName("breadcrumbBack");
    backButton_->setFixedSize(24, 24);
    connect(backButton_, &QPushButton::clicked, this, &BreadcrumbWidget::backRequested);

    pathLabel_ = new QLabel(this);
    pathLabel_->setObjectName("breadcrumbPath");

    dateLabel_ = new QLabel(this);
    dateLabel_->setObjectName("breadcrumbDate");
    dateOpacity_ = new QGraphicsOpacityEffect(dateLabel_);
    dateOpacity_->setOpacity(1.0);
    dateLabel_->setGraphicsEffect(dateOpacity_);

    moreButton_ = new QPushButton(QString::fromUtf8("⋮"), this);
    moreButton_->setObjectName("breadcrumbMore");
    moreButton_->setFixedSize(24, 24);
    connect(moreButton_, &QPushButton::clicked, this, [this] {
        emit moreOptionsRequested(moreButton_->mapToGlobal(moreButton_->rect().bottomRight()));
    });

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(10);
    layout->addWidget(backButton_);
    layout->addWidget(pathLabel_);
    layout->addStretch(1);
    layout->addWidget(dateLabel_);
    layout->addWidget(moreButton_);

    setEmpty();
}

void BreadcrumbWidget::setPath(const QString& path) {
    pathLabel_->setText(path);
}

void BreadcrumbWidget::showSavedPulse(const QDateTime& savedAt) {
    dateLabel_->setText(savedAt.toString("dd MMM yyyy · HH:mm").toUpper());

    // Pulso real de opacidad, no un color estático: confirma visualmente
    // que el autoguardado ocurrió justo ahora.
    auto* animation = new QPropertyAnimation(dateOpacity_, "opacity", this);
    animation->setDuration(500);
    animation->setKeyValueAt(0.0, 1.0);
    animation->setKeyValueAt(0.5, 0.3);
    animation->setKeyValueAt(1.0, 1.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void BreadcrumbWidget::setEmpty() {
    pathLabel_->setText(QObject::tr("Ninguna nota abierta"));
    dateLabel_->setText(QString());
}

void BreadcrumbWidget::contextMenuEvent(QContextMenuEvent* event) {
    emit moreOptionsRequested(event->globalPos());
}

} // namespace noctis::ui
