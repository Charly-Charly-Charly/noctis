#include "UI/WelcomeScreen.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace noctis::ui {

namespace {
constexpr int kIdRole = Qt::UserRole;
} // namespace

WelcomeScreen::WelcomeScreen(core::INoteRepository& repository,
                              core::RecentsService& recentsService, QWidget* parent)
    : QWidget(parent), repository_(repository), recentsService_(recentsService) {
    setObjectName("welcomeScreen");

    auto* titleLabel = new QLabel(tr("Noctis"), this);
    titleLabel->setObjectName("welcomeTitle");
    titleLabel->setAlignment(Qt::AlignCenter);

    auto* subtitleLabel = new QLabel(tr("Editor de notas Markdown local-first"), this);
    subtitleLabel->setObjectName("welcomeSubtitle");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    auto* newNoteButton = new QPushButton(tr("+ Nueva nota"), this);
    newNoteButton->setObjectName("welcomeNewNoteButton");
    newNoteButton->setCursor(Qt::PointingHandCursor);
    connect(newNoteButton, &QPushButton::clicked, this, &WelcomeScreen::newNoteRequested);

    auto* recentsLabel = new QLabel(tr("NOTAS RECIENTES"), this);
    recentsLabel->setObjectName("welcomeRecentsLabel");

    recentsList_ = new QListWidget(this);
    recentsList_->setObjectName("welcomeRecentsList");
    connect(recentsList_, &QListWidget::itemActivated, this, [this](QListWidgetItem* item) {
        emit noteSelected(item->data(kIdRole).toString().toStdString());
    });

    emptyHint_ = new QLabel(tr("Todavía no abriste ninguna nota."), this);
    emptyHint_->setObjectName("sidebarEmptyHint");
    emptyHint_->setAlignment(Qt::AlignCenter);

    auto* column = new QWidget(this);
    column->setObjectName("welcomeColumn");
    column->setMaximumWidth(420);
    auto* columnLayout = new QVBoxLayout(column);
    columnLayout->setSpacing(10);
    columnLayout->addWidget(titleLabel);
    columnLayout->addWidget(subtitleLabel);
    columnLayout->addSpacing(20);
    columnLayout->addWidget(newNoteButton, 0, Qt::AlignHCenter);
    columnLayout->addSpacing(30);
    columnLayout->addWidget(recentsLabel);
    columnLayout->addWidget(recentsList_, 1);
    columnLayout->addWidget(emptyHint_);

    auto* centeredRow = new QHBoxLayout();
    centeredRow->addStretch(1);
    centeredRow->addWidget(column);
    centeredRow->addStretch(1);

    auto* outer = new QVBoxLayout(this);
    outer->addStretch(1);
    outer->addLayout(centeredRow);
    outer->addStretch(2);

    refresh();
}

void WelcomeScreen::refresh() {
    recentsList_->clear();

    for (const core::NoteId& id : recentsService_.listRecents()) {
        auto note = repository_.find(id);
        if (!note) continue; // nota borrada o movida fuera de Noctis desde el último registro

        QString folderName = QString::fromStdString(note->path.parent_path().filename().string());
        auto* item = new QListWidgetItem(
            QString::fromStdString(note->title) + "   ·   " + folderName, recentsList_);
        item->setData(kIdRole, QString::fromStdString(id));
    }

    bool hasRecents = recentsList_->count() > 0;
    recentsList_->setVisible(hasRecents);
    emptyHint_->setVisible(!hasRecents);
}

} // namespace noctis::ui
