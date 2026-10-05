#include "UI/SidebarWidget.h"

#include <QEasingCurve>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QVariantAnimation>

#include "UI/FolderTreeWidget.h"
#include "UI/IconButton.h"
#include "UI/Icons.h"

namespace noctis::ui {

namespace {
constexpr int kCollapsedWidth = 48;
constexpr int kExpandedMinWidth = 200;
constexpr int kWidthAnimationMs = 180;

// Durante la animación el contenido cambia a mitad de camino en vez de
// quedar apretujado hasta el final: por debajo de este ancho se ve la mini
// franja, por encima el contenido normal; el título y los botones del header
// necesitan más espacio todavía para no pisarse.
constexpr int kContentThreshold = 110;
constexpr int kHeaderThreshold = 170;
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
    starLabel_ = new IconLabel(QStringLiteral("star"), 12, this);
    starLabel_->setObjectName("sidebarTitleStar");
    addButton_ = makeIconButton(QStringLiteral("add"), this);
    addButton_->setObjectName("sidebarAddButton");
    addButton_->setFixedSize(28, 28);
    addButton_->setToolTip(tr("Nueva nota"));
    connect(addButton_, &QPushButton::clicked, this,
            [this] { emit createNoteRequested(rootFolder_); });

    collapseButton_ = makeIconButton(QStringLiteral("collapse"), this);
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
    newSpaceButton_->setCursor(Qt::PointingHandCursor);
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
    auto* settingsButton = makeIconButton(QStringLiteral("settings"), this);
    auto* shortcutsButton = makeIconButton(QStringLiteral("keyboard"), this);
    auto* helpButton = makeIconButton(QStringLiteral("help"), this);
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
    auto* railAddButton = makeIconButton(QStringLiteral("add"), this);
    auto* railSettingsButton = makeIconButton(QStringLiteral("settings"), this);
    auto* railShortcutsButton = makeIconButton(QStringLiteral("keyboard"), this);
    auto* railHelpButton = makeIconButton(QStringLiteral("help"), this);
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

    widthAnimation_ = new QVariantAnimation(this);
    widthAnimation_->setDuration(kWidthAnimationMs);
    widthAnimation_->setEasingCurve(QEasingCurve::OutCubic);
    connect(widthAnimation_, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& value) { applyWidth(value.toInt()); });
    connect(widthAnimation_, &QVariantAnimation::finished, this,
            &SidebarWidget::finishWidthAnimation);

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

void SidebarWidget::setCollapsed(bool collapsed, bool animated) {
    if (collapsed_ == collapsed) return;
    collapsed_ = collapsed;

    collapseButton_->setIconName(collapsed ? QStringLiteral("expand") : QStringLiteral("collapse"));
    collapseButton_->setToolTip(collapsed ? tr("Mostrar barra lateral")
                                           : tr("Ocultar barra lateral"));

    widthAnimation_->stop();

    // Al volver a expandir se recupera el ancho que el usuario tenía (puede
    // haberlo arrastrado desde el splitter), no uno fijo.
    if (collapsed && isVisible()) expandedWidth_ = qMax(width(), kExpandedMinWidth);
    const int target = collapsed ? kCollapsedWidth : expandedWidth_;

    if (animated && isVisible()) {
        widthAnimation_->setStartValue(width());
        widthAnimation_->setEndValue(target);
        widthAnimation_->start();
    } else {
        applyWidth(target);
        finishWidthAnimation();
    }

    emit collapsedChanged(collapsed);
}

void SidebarWidget::applyWidth(int width) {
    const bool showContent = width >= kContentThreshold;
    const bool showHeaderExtras = width >= kHeaderThreshold;

    expandedContent_->setVisible(showContent);
    collapsedRail_->setVisible(!showContent);
    titleLabel_->setVisible(showHeaderExtras);
    starLabel_->setVisible(showHeaderExtras);
    addButton_->setVisible(showHeaderExtras);

    setFixedWidth(width);
    emit widthAnimated(width);
}

void SidebarWidget::finishWidthAnimation() {
    // Colapsada queda fija (el splitter no la deja agrandar a mano); expandida
    // vuelve a ser redimensionable. Se suelta primero el máximo para que
    // nunca haya un mínimo mayor que el máximo.
    if (collapsed_) {
        setFixedWidth(kCollapsedWidth);
        return;
    }
    setMaximumWidth(QWIDGETSIZE_MAX);
    setMinimumWidth(kExpandedMinWidth);
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
        tagButton->setCursor(Qt::PointingHandCursor);
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
