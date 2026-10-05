#pragma once

#include <QList>
#include <QPlainTextEdit>
#include <QString>
#include <QTextBlock>
#include <QTextCursor>

#include <memory>

class QAction;
class QImage;
class QMimeData;
class QTimer;

namespace noctis::spelling {
class SpellChecker;
}

namespace noctis::editor {

class SpellCheckHighlighter;

// Widget de edición base. En v1 empieza como texto plano (fase 1 del
// roadmap); el resaltado de sintaxis con Tree-sitter y el folding de
// encabezados se añaden sobre esta clase sin cambiar su contrato público.
//
// Comportamientos de edición propios (QPlainTextEdit no los trae):
//  - Cierre automático de ( [ { " ' ` y envoltura de la selección.
//  - Multicursor con Alt+clic (las ediciones se aplican a todos los cursores).
//  - Viñetas/numeración/checklists automáticas al pulsar Enter.
//  - Tab/Shift+Tab sobre varias líneas (des)indentan todas.
//  - Menú contextual con sugerencias ortográficas y formato (negrita, etc.).
//  - Zoom con Ctrl+rueda (lo aplica quien escucha zoomStepRequested).
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

    // Nivel de zoom relativo al tamaño de fuente original (0 = sin zoom).
    void setZoomLevel(int level);

    // Carpeta real en disco de la nota abierta: de ahí cuelga
    // ".noctis-attachments/" para las imágenes pegadas. Vacío mientras no
    // haya ninguna nota cargada (deshabilita el paste de imágenes).
    void setNoteDirectory(const QString& directory);

    // nullptr deshabilita el subrayado (p. ej. si el diccionario no cargó).
    void setSpellChecker(const spelling::SpellChecker* checker);

    // Quita los cursores adicionales (queda solo el principal).
    void clearExtraCursors();

    // --- Buscar dentro de la nota (Ctrl+F) ------------------------------------
    // Resalta todas las coincidencias (sin distinguir mayúsculas) y selecciona la
    // primera desde el cursor actual. Con texto vacío limpia la búsqueda.
    void setFindQuery(const QString& query);
    // Salta a la coincidencia siguiente/anterior, dando la vuelta al documento.
    void findNext(bool backwards = false);
    void clearFind();

signals:
    void contentChanged(const QString& content);

    // +1 / -1 por cada "paso" de rueda con Ctrl presionado.
    void zoomStepRequested(int direction);

    // `current` es la posición (desde 1) de la coincidencia seleccionada, o 0
    // si el cursor no está sobre ninguna; `total`, cuántas hay en la nota.
    void findResultChanged(int current, int total);

protected:
    bool canInsertFromMimeData(const QMimeData* source) const override;
    void insertFromMimeData(const QMimeData* source) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void handleTextChanged();

private:
    // Guarda la imagen bajo noteDirectory_/.noctis-attachments/ con un
    // nombre único; devuelve la ruta relativa a insertar en el Markdown, o
    // vacío si no se pudo guardar.
    QString saveAttachment(const QImage& image);

    // --- Multicursor -----------------------------------------------------
    // Aplica `edit` al cursor principal y a cada cursor extra dentro de un
    // único paso de deshacer. QTextCursor se reajusta solo cuando otro cursor
    // edita el documento, así que el orden de aplicación da igual.
    template <typename Fn>
    void editAllCursors(Fn&& edit);

    void normalizeCursors();
    void refreshFindMatches();
    void selectFindMatch(int index);
    int currentFindIndex() const;
    void addExtraCursorAt(const QPoint& viewportPos);
    void updateExtraSelections();
    void restartCaretBlink();

    // --- Edición por cursor ---------------------------------------------
    void typeText(QTextCursor& cursor, const QString& text);
    void insertNewline(QTextCursor& cursor);
    void backspace(QTextCursor& cursor);
    void indentLines(bool outdent);
    void moveAllCursors(QTextCursor::MoveOperation operation, bool keepAnchor);
    void toggleFormat(const QString& open, const QString& close);
    void toggleWrap(QTextCursor& cursor, const QString& open, const QString& close);
    bool insideCodeFence(const QTextBlock& block) const;

    void buildFormatActions();

    QString noteDirectory_;
    std::unique_ptr<SpellCheckHighlighter> spellHighlighter_;
    const spelling::SpellChecker* spellChecker_ = nullptr;

    QList<QTextCursor> extraCursors_;
    QString findQuery_;
    QList<QTextCursor> findMatches_;
    QTimer* caretBlink_ = nullptr;
    bool extraCaretVisible_ = true;

    int tabWidth_ = 4;
    int zoomLevel_ = 0;
    qreal baseFontSize_ = 10.0;
    int wheelAccumulator_ = 0;

    QAction* boldAction_ = nullptr;
    QAction* italicAction_ = nullptr;
    QAction* strikeAction_ = nullptr;
    QAction* underlineAction_ = nullptr;
};

} // namespace noctis::editor
