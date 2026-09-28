#pragma once

#include <QPlainTextEdit>
#include <QString>

#include <memory>

class QImage;
class QMimeData;

namespace noctis::spelling {
class SpellChecker;
}

namespace noctis::editor {

class SpellCheckHighlighter;

// Widget de edición base. En v1 empieza como texto plano (fase 1 del
// roadmap); el resaltado de sintaxis con Tree-sitter y el folding de
// encabezados se añaden sobre esta clase sin cambiar su contrato público.
class EditorWidget : public QPlainTextEdit {
    Q_OBJECT

public:
    explicit EditorWidget(QWidget* parent = nullptr);

    // Declarado explícitamente (y definido en el .cpp): spellHighlighter_ es
    // un unique_ptr a un tipo que este header solo declara adelantado, así
    // que su destrucción necesita verlo completo.
    ~EditorWidget() override;

    void setWordWrapEnabled(bool enabled);
    void setTabWidth(int spaces);

    // Carpeta real en disco de la nota abierta: de ahí cuelga
    // ".noctis-attachments/" para las imágenes pegadas. Vacío mientras no
    // haya ninguna nota cargada (deshabilita el paste de imágenes).
    void setNoteDirectory(const QString& directory);

    // nullptr deshabilita el subrayado (p. ej. si el diccionario no cargó).
    void setSpellChecker(const spelling::SpellChecker* checker);

signals:
    void contentChanged(const QString& content);

protected:
    bool canInsertFromMimeData(const QMimeData* source) const override;
    void insertFromMimeData(const QMimeData* source) override;

private slots:
    void handleTextChanged();

private:
    // Guarda la imagen bajo noteDirectory_/.noctis-attachments/ con un
    // nombre único; devuelve la ruta relativa a insertar en el Markdown, o
    // vacío si no se pudo guardar.
    QString saveAttachment(const QImage& image);

    QString noteDirectory_;
    std::unique_ptr<SpellCheckHighlighter> spellHighlighter_;
};

} // namespace noctis::editor
