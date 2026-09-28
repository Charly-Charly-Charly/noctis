#include "UI/PreviewWidget.h"

#include <QUrl>

#include "UI/Theme.h"

namespace noctis::ui {

PreviewWidget::PreviewWidget(QWidget* parent) : QTextBrowser(parent) {
    setOpenExternalLinks(false); // en el futuro, los enlaces son wikilinks internos, no URLs
    document()->setDefaultStyleSheet(theme::previewContentStylesheet(/*dark=*/false));
}

void PreviewWidget::setMarkdownSource(const QString& source) {
    lastSource_ = source;
    setHtml(QString::fromStdString(renderer_.render(source.toStdString())));
}

void PreviewWidget::setBaseDirectory(const QString& directory) {
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
