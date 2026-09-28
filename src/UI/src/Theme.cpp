#include "UI/Theme.h"

#include <QPainter>
#include <QPixmap>
#include <QStringList>

namespace noctis::ui::theme {

namespace {
struct Palette {
    const char* background;
    const char* panel;
    const char* ink;
    const char* inkSoft;
    const char* border;
};

// Claro: crema sobre tinta. Oscuro: la misma pareja invertida.
constexpr Palette kLight{"#ECEEE1", "#F2F3E9", "#1D1D1B", "#4A4A46", "#1D1D1B"};
constexpr Palette kDark{"#1B1C17", "#242520", "#ECEEE1", "#9A9B90", "#ECEEE1"};
} // namespace

QString previewContentStylesheet(bool dark) {
    const Palette& p = dark ? kDark : kLight;
    return QString(R"(
table { border-collapse: collapse; margin: 8px 0; }
th, td { border: 1px solid %1; padding: 4px 10px; }
th { background: %2; font-weight: 600; }
)")
        .arg(p.border, p.panel);
}

TabCloseButtonStyle::TabCloseButtonStyle(bool dark) : dark_(dark) {}

QIcon TabCloseButtonStyle::standardIcon(StandardPixmap standardIcon, const QStyleOption* option,
                                         const QWidget* widget) const {
    if (standardIcon != QStyle::SP_TabCloseButton) {
        return QProxyStyle::standardIcon(standardIcon, option, widget);
    }

    const Palette& p = dark_ ? kDark : kLight;

    constexpr int kSize = 14;
    constexpr int kMargin = 4;
    QPixmap pixmap(kSize, kSize);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen{QColor(p.inkSoft)};
    pen.setWidthF(1.6);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawLine(kMargin, kMargin, kSize - kMargin, kSize - kMargin);
    painter.drawLine(kSize - kMargin, kMargin, kMargin, kSize - kMargin);

    return QIcon(pixmap);
}

QFont applicationFont() {
    QFont font;
    font.setFamilies(QStringList{"Cascadia Mono", "JetBrains Mono", "Consolas", "Courier New"});
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(10);
    // Explícito a propósito: el motor de impresión de Qt (usado también para
    // exportar a PDF) resuelve familias de fuente por un camino distinto al
    // de pantalla, y puede sustituir por una variante Bold si no fija el
    // peso — el texto se ve normal en pantalla pero negrita en el PDF.
    font.setWeight(QFont::Normal);
    return font;
}

QString stylesheet(bool dark) {
    const Palette& p = dark ? kDark : kLight;

    return QString(R"(
QMainWindow, QDialog {
    background: %1;
}

QWidget {
    color: %3;
    font-family: "Cascadia Mono", "Consolas", "Courier New", monospace;
}

QMenuBar {
    background: %1;
    border-bottom: 1px solid %5;
    padding: 2px 4px;
}

QMenuBar::item {
    background: transparent;
    padding: 4px 10px;
}

QMenuBar::item:selected {
    background: %5;
    color: %1;
}

QMenu {
    background: %2;
    border: 1px solid %5;
    padding: 4px;
}

QMenu::item {
    padding: 6px 24px 6px 12px;
}

QMenu::item:selected {
    background: %5;
    color: %1;
    border-radius: 6px;
}

QMenu::separator {
    height: 1px;
    background: %5;
    margin: 4px 6px;
}

QTreeWidget {
    background: %1;
    border: none;
    border-right: 1px solid %5;
    outline: 0;
    padding: 6px 0px;
}

QTreeWidget::item {
    padding: 6px 10px;
    border-bottom: 1px solid %4;
}

QTreeWidget::item:selected {
    /* La nota "activa" ya la resalta TreeRowWidget con su propio estado; la
       selección nativa de Qt (que dispara con clicks fuera de nuestro
       control, como el teclado) no debe pintar nada encima, o el texto del
       row widget (oscuro) queda ilegible sobre este fondo oscuro. */
    background: transparent;
}

QPlainTextEdit, QTextBrowser, QTextEdit {
    background: %1;
    color: %3;
    border: none;
    padding: 16px;
    selection-background-color: %5;
    selection-color: %1;
}

QPlainTextEdit[splitMode="true"] {
    border-right: 1px solid %5;
}

QLineEdit {
    background: %2;
    color: %3;
    border: 1px solid %5;
    border-radius: 18px;
    padding: 9px 18px;
    selection-background-color: %5;
    selection-color: %1;
}

QListWidget {
    background: %2;
    border: 1px solid %5;
    border-radius: 10px;
    padding: 4px;
    outline: 0;
}

QListWidget::item {
    padding: 8px 10px;
    border-radius: 6px;
}

QListWidget::item:selected {
    background: %5;
    color: %1;
}

QListWidget::item:hover:!selected {
    background: %1;
}

QSplitter::handle {
    background: %5;
}

QSplitter::handle:horizontal {
    width: 1px;
}

QSplitter::handle:vertical {
    height: 1px;
}

QPushButton {
    background: %5;
    color: %1;
    border: 1px solid %5;
    border-radius: 14px;
    padding: 6px 18px;
    font-weight: 600;
}

QPushButton:hover {
    background: %4;
    border-color: %4;
}

QPushButton:pressed {
    background: %3;
    border-color: %3;
}

QPushButton:disabled {
    background: %2;
    color: %4;
    border-color: %4;
}

QScrollBar:vertical, QScrollBar:horizontal {
    background: %1;
    border: none;
    margin: 0px;
}

QScrollBar::handle {
    background: %5;
    border-radius: 4px;
    min-height: 24px;
    min-width: 24px;
}

QScrollBar::add-line, QScrollBar::sub-line {
    height: 0px;
    width: 0px;
    background: none;
}

QLabel {
    background: transparent;
}

QStatusBar {
    background: %1;
    border-top: 1px solid %5;
}

QStatusBar QLabel {
    color: %4;
    padding: 0px 8px;
}

/* --- Sidebar --------------------------------------------------------- */

#sidebar {
    background: %1;
    border-right: 1px solid %5;
}

#sidebarTitle {
    font-size: 15px;
    font-weight: 700;
    padding-left: 12px;
}

#sidebarTitleStar {
    color: %5;
}

#sidebarSectionLabel {
    color: %4;
    font-size: 10px;
    font-weight: 700;
    padding: 10px 12px 2px 12px;
}

#sidebarLinkButton, #sidebarTagButton {
    background: transparent;
    border: none;
    color: %4;
    padding: 4px 12px;
    font-weight: 400;
    border-radius: 0px;
}

#sidebarLinkButton:hover, #sidebarTagButton:hover {
    color: %3;
    background: %2;
}

#sidebarEmptyHint {
    color: %4;
    padding: 4px 12px;
    font-size: 11px;
}

#sidebarAddButton, #sidebarIconButton, #sidebarCollapseButton, #breadcrumbBack,
#breadcrumbMore {
    background: transparent;
    color: %3;
    border: 1px solid %5;
    border-radius: 13px;
    padding: 0px;
    font-weight: 400;
}

#sidebarAddButton:hover, #sidebarIconButton:hover, #sidebarCollapseButton:hover,
#breadcrumbBack:hover, #breadcrumbMore:hover {
    background: %5;
    color: %1;
}

/* --- Fila del árbol ---------------------------------------------------- */

#treeRowNumber, #treeRowIcon {
    color: %4;
    font-size: 10px;
}

#treeRowTitle {
    color: %3;
}

#treeRowDot {
    color: #C97A3D;
}

TreeRowWidget[active="true"] {
    background: %5;
    border-radius: 8px;
}

TreeRowWidget[active="true"] #treeRowNumber,
TreeRowWidget[active="true"] #treeRowIcon,
TreeRowWidget[active="true"] #treeRowTitle {
    color: %1;
}

/* --- Pestañas ------------------------------------------------------- */

#tabRow {
    background: %1;
    border-bottom: 1px solid %5;
}

QTabBar {
    background: transparent;
}

QTabBar::tab {
    background: transparent;
    color: %4;
    /* Right más ancho que el resto: dejarle sitio al botón de cerrar, que
       si no queda pegado contra ese borde de la pestaña. */
    padding: 6px 26px 6px 14px;
    border: none;
    border-bottom: 2px solid transparent;
    margin-right: 2px;
}

QTabBar::tab:selected {
    background: %5;
    color: %1;
    border-bottom: 2px solid %5;
    border-top-left-radius: 8px;
    border-top-right-radius: 8px;
    font-weight: 600;
}

QTabBar::tab:hover:!selected {
    color: %3;
}

QTabBar::close-button {
    subcontrol-position: right;
    padding: 2px;
}

/* --- Breadcrumb ------------------------------------------------------- */

#breadcrumb {
    background: %1;
    border-bottom: 1px solid %4;
}

#breadcrumbPath {
    color: %3;
    font-weight: 600;
}

#breadcrumbDate {
    color: %4;
    font-size: 10px;
}

/* --- Pantalla de entrada ------------------------------------------------ */

#welcomeScreen {
    background: %1;
}

#welcomeTitle {
    font-size: 26px;
    font-weight: 700;
}

#welcomeSubtitle {
    color: %4;
    font-size: 12px;
}

#welcomeNewNoteButton {
    padding: 10px 28px;
    font-size: 13px;
}

#welcomeRecentsLabel {
    color: %4;
    font-size: 10px;
    font-weight: 700;
    padding-left: 2px;
}

/* --- Diálogos auxiliares ------------------------------------------------ */

#shortcutKeys {
    background: %2;
    border: 1px solid %5;
    border-radius: 4px;
    padding: 2px 8px;
    color: %3;
}

#helpTitle {
    font-size: 18px;
    font-weight: 700;
}
)")
        .arg(p.background, p.panel, p.ink, p.inkSoft, p.border);
}

} // namespace noctis::ui::theme
