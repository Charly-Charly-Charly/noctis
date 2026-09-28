#pragma once

#include <QTextBrowser>

#include "Markdown/HtmlRenderer.h"

namespace noctis::ui {

// Renderiza Markdown a HTML (vía MD4C) y lo muestra de solo lectura, sobre
// QTextDocument en vez de un motor web completo, para mantener el arranque
// instantáneo y el consumo de RAM bajo.
class PreviewWidget : public QTextBrowser {
public:
    explicit PreviewWidget(QWidget* parent = nullptr);

    void setMarkdownSource(const QString& source);
    void setDarkMode(bool dark);

    // Carpeta real de la nota abierta: sin esto, las imágenes insertadas con
    // rutas relativas (".noctis-attachments/…") no resuelven en el
    // QTextDocument, que no conoce el disco por su cuenta.
    void setBaseDirectory(const QString& directory);

private:
    md::HtmlRenderer renderer_;
    QString lastSource_;
};

} // namespace noctis::ui
