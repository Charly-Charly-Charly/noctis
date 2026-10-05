#pragma once

#include <QString>
#include <QTextBrowser>

#include <memory>

#include "Markdown/HtmlRenderer.h"

class QTextDocument;

namespace noctis::ui {

// Renderiza Markdown a HTML (vía MD4C) y lo muestra de solo lectura, sobre
// QTextDocument en vez de un motor web completo, para mantener el arranque
// instantáneo y el consumo de RAM bajo.
class PreviewWidget : public QTextBrowser {
    Q_OBJECT

public:
    explicit PreviewWidget(QWidget* parent = nullptr);

    void setMarkdownSource(const QString& source);
    void setDarkMode(bool dark);

    // Nivel de zoom relativo al tamaño de fuente original (0 = sin zoom).
    void setZoomLevel(int level);

    // Carpeta real de la nota abierta: sin esto, las imágenes insertadas con
    // rutas relativas (".noctis-attachments/…") no resuelven en el
    // QTextDocument, que no conoce el disco por su cuenta.
    void setBaseDirectory(const QString& directory);

    // Documento independiente de la pantalla, listo para imprimir a PDF:
    // fuente estática (ver comentario en el .cpp), colores del tema claro, e
    // imágenes con ruta absoluta y acotadas al ancho de página. Quien lo
    // imprime es dueño del resultado.
    std::unique_ptr<QTextDocument> createPrintDocument(const QString& markdown) const;

signals:
    // +1 / -1 por cada "paso" de rueda con Ctrl presionado.
    void zoomStepRequested(int direction);

protected:
    void wheelEvent(QWheelEvent* event) override;

private:
    md::HtmlRenderer renderer_;
    QString lastSource_;
    QString baseDirectory_;
    qreal baseFontSize_ = 10.0;
    int wheelAccumulator_ = 0;
};

} // namespace noctis::ui
