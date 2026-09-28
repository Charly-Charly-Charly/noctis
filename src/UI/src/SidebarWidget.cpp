#include "UI/SidebarWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "UI/FolderTreeWidget.h"

namespace noctis::ui {

namespace {
constexpr int kCollapsedWidth = 48;
constexpr int kExpandedMinWidth = 200;
} // namespace

SidebarWidget::SidebarWidget(core::INoteRepository& repository,
                              core::FavoritesService& favoritesService,
                              core::RecentsService& recentsService, core::TagService& tagService,
                              core::ISettingsStore& settings, core::Folder rootFolder,
                              QWidget* parent)
    : QWidget(parent),
      repository_(repository),
      tagService_(tagService),
      settings_(settings),
      rootFolder_(std::move(rootFolder)) {
    setObjectName("sidebar");
    setAttribute(Qt::WA_StyledBackground, true);

    auto* headerRow = new QHBoxLayout();
    titleLabel_ = new QLabel(tr("NOCTIS"), this);
    titleLabel_->setObjectName("sidebarTitle");
    starLabel_ = new QLabel(QString::fromUtf8(" ★"), this);
    starLabel_->setObjectName("sidebarTitleStar");
    addButton_ = new QPushButton(QString::fromUtf8("+"), this);
    addButton_->setObjectName("sidebarAddButton");
    addButton_->setFixedSize(28, 28);
    addButton_->setToolTip(tr("Nueva nota"));
    connect(addButton_, &QPushButton::clicked, this,
            [this] { emit createNoteRequested(rootFolder_); });

    collapseButton_ = new QPushButton(QString::fromUtf8("«"), this);
    collapseButton_->setObjectName("sidebarCollapseButton");
    collapseButton_->setFixedSize(28, 28);
    collapseButton_->setToolTip(tr("Ocultar barra lateral"));
    connect(collapseButton_, &QPushButton::clicked, this,
            [this] { setCollapsed(!collapsed_); });

    // Sin este margen el botón "+" queda pegado al borde derecho del
    // sidebar: addWidget() a un QHBoxLayout suelto no hereda ningún inset.
    headerRow->setContentsMargins(0, 4, 12, 4);
    headerRow->addWidget(titleLabel_);
    headerRow->addWidget(starLabel_);
    headerRow->addStretch(1);
    headerRow->addWidget(addButton_);
    headerRow->addWidget(collapseButton_);

    spacesLabel_ = new QLabel(tr("ESPACIOS"), this);
    spacesLabel_->setObjectName("sidebarSectionLabel");

    tree_ = new FolderTreeWidget(repository, favoritesService, recentsService, rootFolder_, this);
    connect(tree_, &FolderTreeWidget::noteActivated, this, &SidebarWidget::noteActivated);
    connect(tree_, &FolderTreeWidget::createNoteRequested, this,
            &SidebarWidget::createNoteRequested);
    connect(tree_, &FolderTreeWidget::createSubfolderRequested, this,
            &SidebarWidget::createSubfolderRequested);
    connect(tree_, &FolderTreeWidget::renameFolderRequested, this,
            &SidebarWidget::renameFolderRequested);
    connect(tree_, &FolderTreeWidget::deleteFolderRequested, this,
            &SidebarWidget::deleteFolderRequested);
    connect(tree_, &FolderTreeWidget::renameNoteRequested, this,
            &SidebarWidget::renameNoteRequested);
    connect(tree_, &FolderTreeWidget::deleteNoteRequested, this,
            &SidebarWidget::deleteNoteRequested);

    newSpaceButton_ = new QPushButton(tr("+ Nuevo espacio"), this);
    newSpaceButton_->setObjectName("sidebarLinkButton");
    newSpaceButton_->setFlat(true);
    connect(newSpaceButton_, &QPushButton::clicked, this,
            [this] { emit createSubfolderRequested(rootFolder_); });

    tagsLabel_ = new QLabel(tr("ETIQUETAS"), this);
    tagsLabel_->setObjectName("sidebarSectionLabel");

    tagsContainer_ = new QWidget(this);
    tagsLayout_ = new QVBoxLayout(tagsContainer_);
    tagsLayout_->setContentsMargins(0, 0, 0, 0);
    tagsLayout_->setSpacing(2);

    iconRowContainer_ = new QWidget(this);
    auto* iconRow = new QHBoxLayout(iconRowContainer_);
    // Mismo motivo que headerRow: sin esto, los tres botones quedan pegados
    // al borde izquierdo del sidebar.
    iconRow->setContentsMargins(12, 4, 12, 8);
    iconRow->setSpacing(8);
    auto* settingsButton = new QPushButton(QString::fromUtf8("⚙"), this);
    auto* shortcutsButton = new QPushButton(QString::fromUtf8("⌨"), this);
    auto* helpButton = new QPushButton(QString::fromUtf8("?"), this);
    for (QPushButton* button : {settingsButton, shortcutsButton, helpButton}) {
        button->setObjectName("sidebarIconButton");
        button->setFixedSize(28, 28);
    }
    connect(settingsButton, &QPushButton::clicked, this, &SidebarWidget::settingsRequested);
    connect(shortcutsButton, &QPushButton::clicked, this, &SidebarWidget::shortcutsRequested);
    connect(helpButton, &QPushButton::clicked, this, &SidebarWidget::helpRequested);
    iconRow->addWidget(settingsButton);
    iconRow->addWidget(shortcutsButton);
    iconRow->addWidget(helpButton);
    iconRow->addStretch(1);

    expandedContent_ = new QWidget(this);
    auto* expandedLayout = new QVBoxLayout(expandedContent_);
    expandedLayout->setContentsMargins(0, 0, 0, 0);
    expandedLayout->setSpacing(4);
    expandedLayout->addWidget(spacesLabel_);
    expandedLayout->addWidget(tree_, 1);
    expandedLayout->addWidget(newSpaceButton_);
    expandedLayout->addWidget(tagsLabel_);
    expandedLayout->addWidget(tagsContainer_);
    expandedLayout->addWidget(iconRowContainer_);

    // Mini franja que reemplaza al contenido normal al colapsar: mismos
    // botones de ajustes/atajos/ayuda que iconRowContainer_ (no se puede
    // reusar el mismo QPushButton en dos layouts a la vez), apilados
    // verticalmente, más "nueva nota" arriba — todo sigue a un click.
    collapsedRail_ = new QWidget(this);
    auto* railLayout = new QVBoxLayout(collapsedRail_);
    railLayout->setContentsMargins(0, 8, 0, 8);
    railLayout->setSpacing(8);
    auto* railAddButton = new QPushButton(QString::fromUtf8("+"), this);
    auto* railSettingsButton = new QPushButton(QString::fromUtf8("⚙"), this);
    auto* railShortcutsButton = new QPushButton(QString::fromUtf8("⌨"), this);
    auto* railHelpButton = new QPushButton(QString::fromUtf8("?"), this);
    railAddButton->setObjectName("sidebarAddButton");
    railAddButton->setToolTip(tr("Nueva nota"));
    for (QPushButton* button : {railSettingsButton, railShortcutsButton, railHelpButton}) {
        button->setObjectName("sidebarIconButton");
    }
    for (QPushButton* button :
         {railAddButton, railSettingsButton, railShortcutsButton, railHelpButton}) {
        button->setFixedSize(28, 28);
        railLayout->addWidget(button, 0, Qt::AlignHCenter);
    }
    connect(railAddButton, &QPushButton::clicked, this,
            [this] { emit createNoteRequested(rootFolder_); });
    connect(railSettingsButton, &QPushButton::clicked, this, &SidebarWidget::settingsRequested);
    connect(railShortcutsButton, &QPushButton::clicked, this, &SidebarWidget::shortcutsRequested);
    connect(railHelpButton, &QPushButton::clicked, this, &SidebarWidget::helpRequested);
    railLayout->addStretch(1);
    collapsedRail_->setVisible(false);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 8);
    layout->setSpacing(4);
    layout->addLayout(headerRow);
    layout->addWidget(expandedContent_, 1);
    layout->addWidget(collapsedRail_, 1);

    rebuildTags();
    setMinimumWidth(kExpandedMinWidth);
}

void SidebarWidget::refresh() {
    tree_->refresh();
    rebuildTags();
}

void SidebarWidget::refreshTags() {
    rebuildTags();
}

void SidebarWidget::setCollapsed(bool collapsed) {
    if (collapsed_ == collapsed) return;
    collapsed_ = collapsed;

    titleLabel_->setVisible(!collapsed);
    starLabel_->setVisible(!collapsed);
    addButton_->setVisible(!collapsed);
    expandedContent_->setVisible(!collapsed);
    collapsedRail_->setVisible(collapsed);

    collapseButton_->setText(collapsed ? QString::fromUtf8("»") : QString::fromUtf8("«"));
    collapseButton_->setToolTip(collapsed ? tr("Mostrar barra lateral")
                                           : tr("Ocultar barra lateral"));

    // Fijar ancho mín=máx obliga al splitter a respetar la franja angosta;
    // liberarlo (mín normal, máx sin límite) hace que el splitter vuelva a
    // agrandarla para cumplir el nuevo mínimo, sin que MainWindow tenga que
    // saber nada de esto.
    setMinimumWidth(collapsed ? kCollapsedWidth : kExpandedMinWidth);
    setMaximumWidth(collapsed ? kCollapsedWidth : QWIDGETSIZE_MAX);

    emit collapsedChanged(collapsed);
}

void SidebarWidget::rebuildTags() {
    QLayoutItem* child = nullptr;
    while ((child = tagsLayout_->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QMap<QString, int> counts;
    for (const core::TagOccurrence& occurrence : tagService_.collectAll(repository_, rootFolder_)) {
        counts[QString::fromStdString(occurrence.tag)] += 1;
    }

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        auto* tagButton = new QPushButton(QString::fromUtf8("# ") + it.key(), this);
        tagButton->setObjectName("sidebarTagButton");
        tagButton->setFlat(true);
        tagButton->setToolTip(tr("%n nota(s) con esta etiqueta", "", it.value()));
        connect(tagButton, &QPushButton::clicked, this,
                [this, tag = it.key()] { emit tagActivated(tag); });
        tagsLayout_->addWidget(tagButton);
    }

    if (counts.isEmpty()) {
        auto* emptyLabel = new QLabel(tr("Sin etiquetas todavía"), this);
        emptyLabel->setObjectName("sidebarEmptyHint");
        tagsLayout_->addWidget(emptyLabel);
    }
}

} // namespace noctis::ui
