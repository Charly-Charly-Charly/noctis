#include "UI/Theme.h"

#include <QFontDatabase>
#include <QPainter>
#include <QPixmap>
#include <QStringList>
#include <QStyleOption>

namespace noctis::ui::theme {

namespace {
struct Palette {
    const char* background;
    const char* panel;
    const char* ink;
    const char* inkSoft;
    const char* border;
    // Fondo de los elementos al pasar el mouse. En claro es más oscuro que el
    // fondo (#ECEEE1 -> #B9BBB0); en oscuro, un poco más claro.
    const char* hover;
};

// Claro: crema sobre tinta. Oscuro: la misma pareja invertida.
constexpr Palette kLight{"#ECEEE1", "#F2F3E9", "#1D1D1B", "#4A4A46", "#1D1D1B", "#B9BBB0"};
constexpr Palette kDark{"#1B1C17", "#242520", "#ECEEE1", "#9A9B90", "#ECEEE1", "#2E2F29"};
} // namespace

QColor hoverColor(bool dark) {
    return QColor((dark ? kDark : kLight).hover);
}

QString previewContentStylesheet(bool dark) {
    const Palette& p = dark ? kDark : kLight;
    return QString(R"(
/* Qt deja <pre> sin ajuste de línea: un bloque de código con líneas largas (o
   texto sangrado, que Markdown trata como código) se salía de la página. */
pre { white-space: pre-wrap; }
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

    // La pestaña activa se rellena con el color de tinta (texto en el color de
    // fondo), así que la ✕ tiene que invertirse ahí o desaparece sobre ese
    // relleno: antes usaba el mismo gris suave en todas y, en modo claro,
    // quedaba casi ilegible sobre la pestaña oscura. QTabBar marca State_Selected
    // en el botón de la pestaña activa.
    const QStyle::State state = option ? option->state : QStyle::State_None;
    const bool selectedTab = state.testFlag(QStyle::State_Selected);
    const bool hovered =
        state.testFlag(QStyle::State_MouseOver) || state.testFlag(QStyle::State_Raised);
    // Al pasar el mouse el botón se rellena con el color de hover del tema
    // (gris medio en claro, gris oscuro en oscuro): ahí vuelve la tinta normal.
    const char* glyph = hovered ? p.ink : (selectedTab ? p.background : p.ink);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen{QColor(glyph)};
    pen.setWidthF(1.8);
    pen.setCapStyle(Qt::RoundCap);
    painter.setPen(pen);
    painter.drawLine(kMargin, kMargin, kSize - kMargin, kSize - kMargin);
    painter.drawLine(kSize - kMargin, kMargin, kMargin, kSize - kMargin);

    return QIcon(pixmap);
}

QString monospaceFamily() {
    // Una sola familia, la primera instalada de la lista. Darle a Qt una lista
    // de familias de respaldo (QFont::setFamilies, o "font-family: A, B" en el
    // QSS) hace que arme un motor de fuentes combinado que carga las fuentes
    // de respaldo completas: medido en esta app, unos 30 MB de RAM extra
    // (de ~38 a ~68 MB) desde el primer widget con texto. Con una única
    // familia ese costo no existe; los caracteres que falten en la fuente los
    // resuelve igual el reemplazo automático del sistema.
    static const QString family = [] {
        for (const QString& candidate : {QStringLiteral("Cascadia Mono"),
                                         QStringLiteral("JetBrains Mono"),
                                         QStringLiteral("Consolas"),
                                         QStringLiteral("Courier New")}) {
            if (QFontDatabase::hasFamily(candidate)) return candidate;
        }
        return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
    }();
    return family;
}

QFont applicationFont() {
    QFont font(monospaceFamily());
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
    font-family: "%7";
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

/* El resaltado de hover/selección de menús y pestañas lo pinta HoverFade con
   fundido (QSS no puede interpolar estos estados): acá queda sin fondo ni
   cambio de color para no pisarlo. */
QMenuBar::item:selected {
    background: transparent;
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
    background: transparent;
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

/* --- Barra de búsqueda en la nota (Ctrl+F) ----------------------------- */

#findBar {
    background: %1;
    border-bottom: 1px solid %5;
}

#findInput {
    padding: 4px 12px;
    border-radius: 14px;
}

#findCount {
    color: %4;
    font-size: 11px;
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

QScrollBar::handle:hover {
    background: %4;
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

#updateNoticeButton {
    padding: 1px 12px;
    border-radius: 10px;
    font-size: 11px;
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
    background: %6;
}

#sidebarEmptyHint {
    color: %4;
    padding: 4px 12px;
    font-size: 11px;
}

/* Estos botones son IconButton y se dibujan a mano (fundido animado al pasar
   el mouse, que QSS no puede hacer): el stylesheet solo aporta colores y el
   peso de la fuente. */
#sidebarAddButton, #sidebarIconButton, #sidebarCollapseButton, #breadcrumbBack,
#breadcrumbMore, #findButton {
    qproperty-borderColor: %5;
    qproperty-fillColor: %5;
    qproperty-textColor: %3;
    qproperty-hoverTextColor: %1;
    font-weight: 400;
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

#treeRow {
    qproperty-hoverColor: %6;
}

#treeRow[active="true"] {
    background: %5;
    border-radius: 8px;
}

#treeRow[active="true"] #treeRowNumber,
#treeRow[active="true"] #treeRowIcon,
#treeRow[active="true"] #treeRowTitle {
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

QTabBar::close-button {
    subcontrol-position: right;
    padding: 2px;
    border-radius: 4px;
}

QTabBar::close-button:hover {
    background: %6;
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
        .arg(p.background, p.panel, p.ink, p.inkSoft, p.border, p.hover, monospaceFamily());
}

} // namespace noctis::ui::theme
