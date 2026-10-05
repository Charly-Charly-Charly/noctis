#include "UI/PreviewWidget.h"

#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QRegularExpression>
#include <QTextDocument>
#include <QUrl>
#include <QWheelEvent>

#include "UI/Theme.h"

namespace noctis::ui {

namespace {

// Ancho máximo de imagen. Una página A4 con los márgenes por defecto de
// QPdfWriter (10 mm) mide ~718 px lógicos (96 dpi, la unidad de QTextDocument);
// se descuentan los 4 px de margen del documento a cada lado y algo de holgura.
constexpr int kPrintContentWidth = 700;

// Reescribe los `src` relativos de las <img> a URLs de archivo absolutas (el
// documento de impresión no sabe en qué carpeta está la nota) y fija el ancho
// de las que no caben en la página: QTextDocument no las reduce solo y se
// cortarían en el borde.
QString withResolvedImages(const QString& html, const QString& baseDirectory) {
    static const QRegularExpression imageSource(
        QStringLiteral(R"((<img\s[^>]*?src=")([^"]*)("))"));

    QString result;
    qsizetype last = 0;
    auto matches = imageSource.globalMatch(html);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        result += html.mid(last, match.capturedStart() - last);
        last = match.capturedEnd();

        // md4c escapa el atributo como HTML y la ruta como URL.
        QString source = match.captured(2);
        source.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
        const QUrl parsed(source);
        QString localPath;
        if (parsed.isRelative()) {
            localPath = QDir(baseDirectory).absoluteFilePath(QUrl::fromPercentEncoding(source.toUtf8()));
        } else if (parsed.isLocalFile()) {
            localPath = parsed.toLocalFile();
        }

        if (localPath.isEmpty() || !QFileInfo::exists(localPath)) {
            result += match.captured(0); // remota, data: o inexistente: sin tocar
            continue;
        }
        result += match.captured(1) +
                  QUrl::fromLocalFile(localPath).toString(QUrl::FullyEncoded) + match.captured(3);
        if (QImageReader(localPath).size().width() > kPrintContentWidth) {
            result += QStringLiteral(" width=\"%1\"").arg(kPrintContentWidth);
        }
    }
    result += html.mid(last);
    return result;
}

} // namespace

PreviewWidget::PreviewWidget(QWidget* parent) : QTextBrowser(parent) {
    setOpenExternalLinks(false); // en el futuro, los enlaces son wikilinks internos, no URLs
    document()->setDefaultStyleSheet(theme::previewContentStylesheet(/*dark=*/false));
    if (font().pointSizeF() > 0) baseFontSize_ = font().pointSizeF();
}

void PreviewWidget::setZoomLevel(int level) {
    QFont zoomed = font();
    zoomed.setPointSizeF(qMax<qreal>(1.0, baseFontSize_ + level));
    setFont(zoomed);
}

void PreviewWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        // El zoom lo gestiona MainWindow para que editor y vista previa
        // queden siempre al mismo nivel; acá solo se traduce la rueda a pasos.
        wheelAccumulator_ += event->angleDelta().y();
        while (wheelAccumulator_ >= 120) {
            emit zoomStepRequested(+1);
            wheelAccumulator_ -= 120;
        }
        while (wheelAccumulator_ <= -120) {
            emit zoomStepRequested(-1);
            wheelAccumulator_ += 120;
        }
        event->accept();
        return;
    }
    QTextBrowser::wheelEvent(event);
}

void PreviewWidget::setMarkdownSource(const QString& source) {
    lastSource_ = source;
    setHtml(QString::fromStdString(renderer_.render(source.toStdString())));
}

std::unique_ptr<QTextDocument> PreviewWidget::createPrintDocument(const QString& markdown) const {
    auto document = std::make_unique<QTextDocument>();

    // "Cascadia Mono" (primera de la lista de applicationFont()) es una
    // fuente variable: Windows la expone con instancias con nombre propio
    // para los pesos livianos (Light, SemiBold, SemiLight...) pero sin una
    // "Cascadia Mono Bold" separada, y el motor de fuentes de Qt para
    // embeber/imprimir PDF no siempre resuelve bien cuál instancia es la
    // "Regular" — termina usando una más gruesa aunque en pantalla se vea
    // normal. Para el PDF se fuerza Consolas, una fuente estática de Windows
    // con caras Regular/Bold bien definidas. Se fija ANTES de cargar el HTML:
    // hacerlo después no actualiza el peso ya grabado en cada fragmento.
    // Tamaño base, sin el zoom de pantalla.
    QFont printFont = font();
    printFont.setFamilies({"Consolas", "Courier New"});
    printFont.setWeight(QFont::Normal);
    printFont.setPointSizeF(baseFontSize_);
    document->setDefaultFont(printFont);

    // Siempre colores del tema claro: el papel es blanco aunque la app esté en
    // modo oscuro (bordes de tabla cremas serían invisibles).
    document->setDefaultStyleSheet(theme::previewContentStylesheet(/*dark=*/false));
    document->setBaseUrl(QUrl::fromLocalFile(baseDirectory_.isEmpty() ? QString()
                                                                       : baseDirectory_ + '/'));

    const QString html = QString::fromStdString(renderer_.render(markdown.toStdString()));
    document->setHtml(withResolvedImages(html, baseDirectory_));
    return document;
}

void PreviewWidget::setBaseDirectory(const QString& directory) {
    baseDirectory_ = directory;
    QString path = directory;
    if (!path.isEmpty() && !path.endsWith('/')) path += '/';
    document()->setBaseUrl(QUrl::fromLocalFile(path));
}

void PreviewWidget::setDarkMode(bool dark) {
    // setDefaultStyleSheet no reformatea el HTML ya cargado: hay que
    // reaplicarlo para que las tablas tomen los nuevos colores.
    document()->setDefaultStyleSheet(theme::previewContentStylesheet(dark));
    setMarkdownSource(lastSource_);
}

} // namespace noctis::ui
