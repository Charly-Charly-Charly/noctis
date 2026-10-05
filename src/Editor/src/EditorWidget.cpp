#include "Editor/EditorWidget.h"

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QImage>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextDocument>
#include <QTextEdit>
#include <QTimer>
#include <QUuid>
#include <QWheelEvent>

#include <map>
#include <optional>
#include <set>

#include "Editor/SpellCheckHighlighter.h"
#include "Spelling/SpellChecker.h"

namespace noctis::editor {

namespace {

constexpr const char* kAttachmentsDirName = ".noctis-attachments";

// QTextDocument::characterAt devuelve QChar() fuera de rango y
// QChar::ParagraphSeparator en cada salto de bloque.
QChar charAt(const QTextDocument* doc, int position) {
    if (position < 0 || position >= doc->characterCount()) return QChar();
    return doc->characterAt(position);
}

bool isBoundary(QChar ch) {
    return ch.isNull() || ch.isSpace();
}

bool isQuote(QChar ch) {
    return ch == QLatin1Char('"') || ch == QLatin1Char('\'') || ch == QLatin1Char('`');
}

// Par de cierre automático para ( [ { " ' `.
QChar closerFor(QChar open) {
    switch (open.unicode()) {
    case '(': return QLatin1Char(')');
    case '[': return QLatin1Char(']');
    case '{': return QLatin1Char('}');
    case '"': return QLatin1Char('"');
    case '\'': return QLatin1Char('\'');
    case '`': return QLatin1Char('`');
    default: return QChar();
    }
}

// Con una selección, además de los pares anteriores se puede envolver con los
// marcadores de énfasis de Markdown.
QChar surroundCloserFor(QChar open) {
    QChar closer = closerFor(open);
    if (!closer.isNull()) return closer;
    if (open == QLatin1Char('*') || open == QLatin1Char('_') || open == QLatin1Char('~')) {
        return open;
    }
    return QChar();
}

// Prefijo de ítem de lista: sangría, viñeta o número, espacio y checkbox
// opcional ("- [ ] ", "1. ", "  * "...).
const QRegularExpression& listPrefixPattern() {
    static const QRegularExpression pattern(QStringLiteral(R"(^(\s*)([-*+]|\d+[.)])(\s+)(\[[ xX]\]\s+)?)"));
    return pattern;
}

// Línea que solo tiene el prefijo de lista (ítem vacío).
const QRegularExpression& emptyListItemPattern() {
    static const QRegularExpression pattern(
        QStringLiteral(R"(^(\s*)([-*+]|\d+[.)])\s+(\[[ xX]\]\s*)?$)"));
    return pattern;
}

const QRegularExpression& wordPattern() {
    static const QRegularExpression pattern(QStringLiteral("[A-Za-zÀ-ÖØ-öø-ÿ]+"));
    return pattern;
}

// Posición (bloque + columna) de un extremo del cursor: sobrevive a ediciones
// que añaden o quitan caracteres dentro de las líneas sin cambiar su número.
struct BlockPosition {
    int block = 0;
    int column = 0;
};

BlockPosition blockPositionOf(const QTextDocument* doc, int position) {
    QTextBlock block = doc->findBlock(position);
    return {block.blockNumber(), position - block.position()};
}

int absolutePosition(const QTextDocument* doc, const BlockPosition& where) {
    QTextBlock block = doc->findBlockByNumber(where.block);
    return block.position() + qMin(where.column, qMax(0, block.length() - 1));
}

} // namespace

EditorWidget::EditorWidget(QWidget* parent) : QPlainTextEdit(parent) {
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setCursorWidth(2);
    baseFontSize_ = font().pointSizeF() > 0 ? font().pointSizeF() : 10.0;
    connect(this, &QPlainTextEdit::textChanged, this, &EditorWidget::handleTextChanged);

    caretBlink_ = new QTimer(this);
    const int flashTime = QApplication::cursorFlashTime();
    caretBlink_->setInterval(flashTime > 0 ? flashTime / 2 : 500);
    connect(caretBlink_, &QTimer::timeout, this, [this] {
        // Con cursorFlashTime() == 0 el sistema pide un cursor sin parpadeo.
        if (QApplication::cursorFlashTime() > 0) extraCaretVisible_ = !extraCaretVisible_;
        viewport()->update();
    });

    buildFormatActions();
}

EditorWidget::~EditorWidget() = default;

void EditorWidget::buildFormatActions() {
    auto makeAction = [this](const QString& text, const QKeySequence& shortcut,
                             const QString& open, const QString& close) {
        auto* action = new QAction(text, this);
        action->setShortcut(shortcut);
        // Solo con el foco en el editor: Ctrl+B/I/U no deben dispararse desde
        // el árbol de notas ni desde la vista previa.
        action->setShortcutContext(Qt::WidgetShortcut);
        connect(action, &QAction::triggered, this,
                [this, open, close] { toggleFormat(open, close); });
        addAction(action);
        return action;
    };
    boldAction_ = makeAction(tr("Negrita"), QKeySequence(QStringLiteral("Ctrl+B")),
                             QStringLiteral("**"), QStringLiteral("**"));
    italicAction_ = makeAction(tr("Cursiva"), QKeySequence(QStringLiteral("Ctrl+I")),
                               QStringLiteral("*"), QStringLiteral("*"));
    strikeAction_ = makeAction(tr("Tachado"), QKeySequence(QStringLiteral("Ctrl+Shift+X")),
                               QStringLiteral("~~"), QStringLiteral("~~"));
    underlineAction_ = makeAction(tr("Subrayado"), QKeySequence(QStringLiteral("Ctrl+U")),
                                  QStringLiteral("<u>"), QStringLiteral("</u>"));
}

void EditorWidget::handleTextChanged() {
    if (!extraCursors_.isEmpty()) normalizeCursors();
    // Las coincidencias de la búsqueda activa quedan desfasadas al editar.
    if (!findQuery_.isEmpty()) refreshFindMatches();
    if (!extraCursors_.isEmpty() || !findQuery_.isEmpty()) updateExtraSelections();
    if (!findQuery_.isEmpty()) {
        emit findResultChanged(currentFindIndex(), static_cast<int>(findMatches_.size()));
    }
    emit contentChanged(toPlainText());
}

void EditorWidget::setWordWrapEnabled(bool enabled) {
    setLineWrapMode(enabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

void EditorWidget::setTabWidth(int spaces) {
    tabWidth_ = spaces;
    setTabStopDistance(spaces * fontMetrics().horizontalAdvance(QLatin1Char(' ')));
}

void EditorWidget::setZoomLevel(int level) {
    zoomLevel_ = level;
    QFont zoomed = font();
    zoomed.setPointSizeF(qMax<qreal>(1.0, baseFontSize_ + level));
    setFont(zoomed);
    // El ancho del tabulador se mide en píxeles: depende de la fuente.
    setTabWidth(tabWidth_);
}

void EditorWidget::setNoteDirectory(const QString& directory) {
    noteDirectory_ = directory;
}

void EditorWidget::setSpellChecker(const spelling::SpellChecker* checker) {
    spellChecker_ = checker;
    spellHighlighter_.reset();
    if (checker) {
        spellHighlighter_ = std::make_unique<SpellCheckHighlighter>(document(), *checker);
    }
}

// --- Multicursor ---------------------------------------------------------

void EditorWidget::clearExtraCursors() {
    extraCursors_.clear();
    restartCaretBlink();
    updateExtraSelections();
}

// --- Buscar dentro de la nota ------------------------------------------------

void EditorWidget::refreshFindMatches() {
    findMatches_.clear();
    if (findQuery_.isEmpty()) return;
    QTextCursor match(document());
    while (true) {
        match = document()->find(findQuery_, match);
        if (match.isNull()) break;
        findMatches_.append(match);
    }
}

int EditorWidget::currentFindIndex() const {
    const QTextCursor cursor = textCursor();
    if (!cursor.hasSelection()) return 0;
    for (qsizetype i = 0; i < findMatches_.size(); ++i) {
        if (findMatches_.at(i).selectionStart() == cursor.selectionStart() &&
            findMatches_.at(i).selectionEnd() == cursor.selectionEnd()) {
            return static_cast<int>(i) + 1;
        }
    }
    return 0;
}

void EditorWidget::selectFindMatch(int index) {
    setTextCursor(findMatches_.at(index));
    updateExtraSelections();
    emit findResultChanged(currentFindIndex(), static_cast<int>(findMatches_.size()));
}

void EditorWidget::setFindQuery(const QString& query) {
    findQuery_ = query;
    refreshFindMatches();

    if (findMatches_.isEmpty()) {
        updateExtraSelections();
        emit findResultChanged(0, 0);
        return;
    }
    // Búsqueda incremental: parte desde donde estaba el cursor, de modo que
    // seguir escribiendo no salte a otra parte del documento.
    const int from = textCursor().selectionStart();
    int target = 0; // si ninguna empieza en `from` o después, vuelve al principio
    for (qsizetype i = 0; i < findMatches_.size(); ++i) {
        if (findMatches_.at(i).selectionStart() >= from) {
            target = static_cast<int>(i);
            break;
        }
    }
    selectFindMatch(target);
}

void EditorWidget::findNext(bool backwards) {
    if (findMatches_.isEmpty()) return;

    const QTextCursor cursor = textCursor();
    const int total = static_cast<int>(findMatches_.size());
    int target = backwards ? total - 1 : 0; // por defecto, la vuelta al otro extremo
    if (backwards) {
        for (int i = total - 1; i >= 0; --i) {
            if (findMatches_.at(i).selectionStart() < cursor.selectionStart()) {
                target = i;
                break;
            }
        }
    } else {
        const int after = cursor.hasSelection() ? cursor.selectionEnd() : cursor.position();
        for (int i = 0; i < total; ++i) {
            if (findMatches_.at(i).selectionStart() >= after) {
                target = i;
                break;
            }
        }
    }
    selectFindMatch(target);
}

void EditorWidget::clearFind() {
    if (findQuery_.isEmpty() && findMatches_.isEmpty()) return;
    findQuery_.clear();
    findMatches_.clear();
    updateExtraSelections();
    emit findResultChanged(0, 0);
}

void EditorWidget::restartCaretBlink() {
    extraCaretVisible_ = true;
    if (extraCursors_.isEmpty()) {
        caretBlink_->stop();
    } else {
        caretBlink_->start();
    }
    viewport()->update();
}

void EditorWidget::updateExtraSelections() {
    QList<QTextEdit::ExtraSelection> selections;

    // Búsqueda: todas las coincidencias en amarillo; la que está seleccionada,
    // en naranja (la selección nativa se ve apagada si el foco está en la
    // barra de búsqueda, y así no depende del estado de foco).
    const QTextCursor primary = textCursor();
    for (const QTextCursor& match : findMatches_) {
        QTextEdit::ExtraSelection selection;
        selection.cursor = match;
        const bool isCurrent = primary.hasSelection() &&
                               primary.selectionStart() == match.selectionStart() &&
                               primary.selectionEnd() == match.selectionEnd();
        selection.format.setBackground(isCurrent ? QColor(255, 140, 0, 170)
                                                 : QColor(255, 213, 79, 110));
        selections.append(selection);
    }

    for (const QTextCursor& cursor : extraCursors_) {
        if (!cursor.hasSelection()) continue;
        QTextEdit::ExtraSelection selection;
        selection.cursor = cursor;
        selection.format.setBackground(palette().highlight());
        selection.format.setForeground(palette().highlightedText());
        selections.append(selection);
    }
    setExtraSelections(selections);
}

// Descarta los cursores extra que se pisan entre sí o con el principal (por
// ejemplo tras borrar texto que los hacía converger): editar dos veces la
// misma posición duplicaría el texto.
void EditorWidget::normalizeCursors() {
    if (extraCursors_.isEmpty()) return;

    auto overlaps = [](const QTextCursor& a, const QTextCursor& b) {
        const int aStart = a.selectionStart(), aEnd = a.selectionEnd();
        const int bStart = b.selectionStart(), bEnd = b.selectionEnd();
        if (aStart == aEnd && bStart == bEnd) return aStart == bStart;
        if (aStart == aEnd) return aStart >= bStart && aStart <= bEnd;
        if (bStart == bEnd) return bStart >= aStart && bStart <= aEnd;
        return aStart < bEnd && bStart < aEnd;
    };

    const QTextCursor primary = textCursor();
    QList<QTextCursor> kept;
    for (const QTextCursor& candidate : extraCursors_) {
        bool drop = overlaps(candidate, primary);
        for (const QTextCursor& other : kept) {
            if (drop) break;
            drop = overlaps(candidate, other);
        }
        if (!drop) kept.append(candidate);
    }
    if (kept.size() != extraCursors_.size()) {
        extraCursors_ = kept;
        restartCaretBlink();
    }
}

void EditorWidget::addExtraCursorAt(const QPoint& viewportPos) {
    normalizeCursors();
    QTextCursor cursor = cursorForPosition(viewportPos);

    // Alt+clic sobre un cursor extra existente lo quita.
    for (qsizetype i = 0; i < extraCursors_.size(); ++i) {
        const QTextCursor& existing = extraCursors_.at(i);
        if (!existing.hasSelection() && existing.position() == cursor.position()) {
            extraCursors_.removeAt(i);
            restartCaretBlink();
            updateExtraSelections();
            return;
        }
    }
    const QTextCursor primary = textCursor();
    if (!primary.hasSelection() && primary.position() == cursor.position()) return;

    extraCursors_.append(cursor);
    normalizeCursors();
    restartCaretBlink();
    updateExtraSelections();
}

template <typename Fn>
void EditorWidget::editAllCursors(Fn&& edit) {
    normalizeCursors();
    QTextCursor primary = textCursor();
    primary.beginEditBlock(); // un solo "deshacer" para todos los cursores
    edit(primary);
    for (QTextCursor& cursor : extraCursors_) edit(cursor);
    primary.endEditBlock();

    setTextCursor(primary);
    normalizeCursors();
    updateExtraSelections();
    ensureCursorVisible();
    restartCaretBlink();
}

void EditorWidget::moveAllCursors(QTextCursor::MoveOperation operation, bool keepAnchor) {
    auto move = [&](QTextCursor& cursor) {
        const bool horizontal =
            operation == QTextCursor::Left || operation == QTextCursor::Right;
        if (!keepAnchor && horizontal && cursor.hasSelection()) {
            // Con una selección, Izq/Der la colapsan en el extremo
            // correspondiente en vez de mover un carácter más.
            cursor.setPosition(operation == QTextCursor::Left ? cursor.selectionStart()
                                                              : cursor.selectionEnd());
            return;
        }
        cursor.movePosition(operation, keepAnchor ? QTextCursor::KeepAnchor
                                                  : QTextCursor::MoveAnchor);
    };

    QTextCursor primary = textCursor();
    move(primary);
    for (QTextCursor& cursor : extraCursors_) move(cursor);
    setTextCursor(primary);
    normalizeCursors();
    updateExtraSelections();
    ensureCursorVisible();
    restartCaretBlink();
}

// --- Edición por cursor ----------------------------------------------------

void EditorWidget::typeText(QTextCursor& cursor, const QString& text) {
    const QChar ch = text.at(0);
    const QTextDocument* doc = cursor.document();

    if (cursor.hasSelection()) {
        const QChar closer = surroundCloserFor(ch);
        if (closer.isNull()) {
            cursor.insertText(text); // sin par: reemplaza la selección, como siempre
            return;
        }
        // Envuelve la selección y la deja seleccionada para poder encadenar
        // (p. ej. comillas dentro de paréntesis).
        const int start = cursor.selectionStart();
        const int end = cursor.selectionEnd();
        cursor.setPosition(end);
        cursor.insertText(QString(closer));
        cursor.setPosition(start);
        cursor.insertText(text);
        cursor.setPosition(start + 1);
        cursor.setPosition(end + 1, QTextCursor::KeepAnchor);
        return;
    }

    const int position = cursor.position();
    const QChar next = charAt(doc, position);
    const QChar prev = charAt(doc, position - 1);
    const bool closing = ch == QLatin1Char(')') || ch == QLatin1Char(']') || ch == QLatin1Char('}');

    // Escribir el cierre que ya está justo adelante: se salta en vez de duplicarlo.
    if ((closing || isQuote(ch)) && next == ch) {
        cursor.movePosition(QTextCursor::Right);
        return;
    }

    const QChar closer = closerFor(ch);
    if (!closer.isNull() && !closing) {
        // Solo se cierra antes de un espacio/fin de línea o de otro cierre,
        // para no meter un par en medio de una palabra ya escrita.
        const bool nextAllows = isBoundary(next) || QStringLiteral(")]},.;:>").contains(next);
        // Comillas/backtick: no tras una letra (apóstrofes, "l'agua") ni tras
        // la misma comilla (``` abre un bloque de código, no se duplica).
        const bool prevAllows = !isQuote(ch) || (!prev.isLetterOrNumber() && prev != ch);
        if (nextAllows && prevAllows) {
            cursor.insertText(text + closer);
            cursor.movePosition(QTextCursor::Left);
            return;
        }
    }
    cursor.insertText(text);
}

bool EditorWidget::insideCodeFence(const QTextBlock& block) const {
    int fences = 0;
    for (QTextBlock b = block.previous(); b.isValid(); b = b.previous()) {
        const QString trimmed = b.text().trimmed();
        if (trimmed.startsWith(QStringLiteral("```")) || trimmed.startsWith(QStringLiteral("~~~"))) {
            ++fences;
        }
    }
    return fences % 2 == 1;
}

void EditorWidget::insertNewline(QTextCursor& cursor) {
    if (cursor.hasSelection()) cursor.removeSelectedText();

    const QTextBlock block = cursor.block();
    const QString lineText = block.text();
    const QRegularExpressionMatch match = listPrefixPattern().match(lineText);
    const int column = cursor.positionInBlock();

    // Enter con el cursor antes de la viñeta, o dentro de un bloque de código,
    // es un salto de línea normal.
    if (!match.hasMatch() || column < match.capturedLength() || insideCodeFence(block)) {
        cursor.insertBlock();
        return;
    }

    // Ítem vacío + Enter: la lista termina (se borra la viñeta).
    if (lineText.mid(match.capturedLength()).trimmed().isEmpty()) {
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        return;
    }

    QString marker = match.captured(2);
    if (marker.front().isDigit()) {
        // Numerada ("3." / "3)"): el siguiente ítem lleva el número + 1.
        const QChar delimiter = marker.back();
        marker = QString::number(marker.chopped(1).toULongLong() + 1) + delimiter;
    }
    QString checkbox = match.captured(4);
    if (!checkbox.isEmpty()) {
        checkbox = QStringLiteral("[ ]") + checkbox.mid(3); // el ítem nuevo empieza sin marcar
    }

    cursor.insertBlock();
    cursor.insertText(match.captured(1) + marker + match.captured(3) + checkbox);
}

void EditorWidget::backspace(QTextCursor& cursor) {
    if (cursor.hasSelection()) {
        cursor.removeSelectedText();
        return;
    }

    const QTextDocument* doc = cursor.document();
    const int position = cursor.position();
    const QChar prev = charAt(doc, position - 1);
    const QChar next = charAt(doc, position);

    // Borrar el abridor de un par vacío "()" se lleva también el cierre.
    if (!next.isNull() && closerFor(prev) == next) {
        cursor.deletePreviousChar();
        cursor.deleteChar();
        return;
    }

    // Ítem de lista vacío: Backspace quita la viñeta completa (no solo el espacio).
    const QTextBlock block = cursor.block();
    if (cursor.positionInBlock() == block.text().size()) {
        const QRegularExpressionMatch match = emptyListItemPattern().match(block.text());
        if (match.hasMatch()) {
            cursor.setPosition(block.position() + match.captured(1).size());
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            return;
        }
    }
    cursor.deletePreviousChar();
}

// Tab / Shift+Tab. Con una selección de varias líneas (des)indenta todas; sin
// selección, Tab inserta un tabulador salvo sobre una línea de lista, donde
// anida el ítem. Shift+Tab siempre quita sangría de las líneas tocadas.
void EditorWidget::indentLines(bool outdent) {
    normalizeCursors();
    QTextDocument* doc = document();

    QTextCursor primary = textCursor();
    QList<QTextCursor*> cursors{&primary};
    for (QTextCursor& cursor : extraCursors_) cursors.append(&cursor);

    // Qué cursores operan sobre líneas enteras y cuáles solo insertan un tab.
    std::set<int> lineNumbers;
    QList<bool> operatesOnLines;
    for (QTextCursor* cursor : cursors) {
        const QTextBlock firstBlock = doc->findBlock(cursor->selectionStart());
        const QTextBlock lastBlock = doc->findBlock(cursor->selectionEnd());
        int first = firstBlock.blockNumber();
        int last = lastBlock.blockNumber();
        const bool multiLine = cursor->hasSelection() && first != last;
        // Una selección que termina justo al inicio de una línea no la toca.
        if (multiLine && cursor->selectionEnd() == lastBlock.position()) --last;

        const bool onListLine =
            !cursor->hasSelection() && listPrefixPattern().match(firstBlock.text()).hasMatch();
        const bool lines = outdent || multiLine || onListLine;
        operatesOnLines.append(lines);
        if (lines) {
            for (int number = first; number <= last; ++number) lineNumbers.insert(number);
        }
    }

    primary.beginEditBlock();

    // 1) Cursores que solo insertan un tabulador (reemplazan su selección).
    for (qsizetype i = 0; i < cursors.size(); ++i) {
        if (!operatesOnLines[i]) cursors[i]->insertText(QStringLiteral("\t"));
    }

    // 2) Capturar los extremos de los cursores de líneas *después* del paso
    // anterior (que pudo desplazarlos) y antes de tocar la sangría.
    struct Saved {
        BlockPosition anchor;
        BlockPosition position;
        bool hasSelection;
        bool anchorFirst;
    };
    std::map<qsizetype, Saved> saved;
    for (qsizetype i = 0; i < cursors.size(); ++i) {
        if (!operatesOnLines[i]) continue;
        saved[i] = {blockPositionOf(doc, cursors[i]->anchor()),
                    blockPositionOf(doc, cursors[i]->position()), cursors[i]->hasSelection(),
                    cursors[i]->anchor() <= cursors[i]->position()};
    }

    // 3) Cambiar la sangría, una vez por línea aunque varios cursores la compartan.
    std::map<int, int> columnDelta; // número de línea -> caracteres añadidos (+) o quitados (-)
    for (int number : lineNumbers) {
        const QTextBlock block = doc->findBlockByNumber(number);
        QTextCursor edit(block);
        if (!outdent) {
            edit.insertText(QStringLiteral("\t"));
            columnDelta[number] = 1;
            continue;
        }
        const QString text = block.text();
        int toRemove = 0;
        if (text.startsWith(QLatin1Char('\t'))) {
            toRemove = 1;
        } else {
            while (toRemove < tabWidth_ && toRemove < text.size() &&
                   text.at(toRemove) == QLatin1Char(' ')) {
                ++toRemove;
            }
        }
        for (int k = 0; k < toRemove; ++k) edit.deleteChar();
        columnDelta[number] = -toRemove;
    }

    // 4) Restaurar las selecciones con las columnas corridas.
    for (const auto& [index, state] : saved) {
        auto shifted = [&](const BlockPosition& where, bool isSelectionStart) {
            const auto it = columnDelta.find(where.block);
            const int delta = it == columnDelta.end() ? 0 : it->second;
            if (delta > 0) {
                // El inicio de una selección en columna 0 se queda ahí para
                // que la sangría nueva siga dentro de lo seleccionado.
                if (state.hasSelection && isSelectionStart && where.column == 0) return 0;
                return where.column + delta;
            }
            return qMax(0, where.column + delta);
        };
        const int anchorColumn = shifted(state.anchor, state.anchorFirst);
        const int positionColumn = shifted(state.position, !state.anchorFirst);
        cursors[index]->setPosition(absolutePosition(doc, {state.anchor.block, anchorColumn}));
        cursors[index]->setPosition(absolutePosition(doc, {state.position.block, positionColumn}),
                                    QTextCursor::KeepAnchor);
    }

    primary.endEditBlock();
    setTextCursor(primary);
    normalizeCursors();
    updateExtraSelections();
    ensureCursorVisible();
    restartCaretBlink();
}

// Marcadores formados por un mismo carácter repetido (* _ ~) se confunden
// entre sí si solo se miran los extremos: "**x**" empieza y termina con "*"
// pero para la cursiva está en negrita. Se cuenta la racha de marcadores.
void EditorWidget::toggleFormat(const QString& open, const QString& close) {
    editAllCursors([&](QTextCursor& cursor) { toggleWrap(cursor, open, close); });
}

void EditorWidget::toggleWrap(QTextCursor& cursor, const QString& open, const QString& close) {
    const QTextDocument* doc = cursor.document();
    const int n = static_cast<int>(open.size());
    const int m = static_cast<int>(close.size());

    if (!cursor.hasSelection()) {
        // Sin selección: aplica a la palabra bajo el cursor.
        const QTextBlock block = cursor.block();
        const QString text = block.text();
        const int column = cursor.positionInBlock();
        int start = column;
        int end = column;
        while (start > 0 && text.at(start - 1).isLetterOrNumber()) --start;
        while (end < text.size() && text.at(end).isLetterOrNumber()) ++end;
        if (start == end) {
            // Nada que envolver: deja el par vacío y el cursor en el medio.
            cursor.insertText(open + close);
            cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, m);
            return;
        }
        cursor.setPosition(block.position() + start);
        cursor.setPosition(block.position() + end, QTextCursor::KeepAnchor);
    }

    const int start = cursor.selectionStart();
    const int end = cursor.selectionEnd();
    auto textAt = [&](int from, int length) {
        QString result;
        for (int i = 0; i < length; ++i) result.append(charAt(doc, from + i));
        return result;
    };

    const bool repeated = open == close && !open.isEmpty() && open.count(open.at(0)) == n;
    auto runLength = [&](int from, int step) {
        int count = 0;
        while (charAt(doc, from + step * count) == open.at(0)) ++count;
        return count;
    };
    // ¿Una racha de `run` marcadores iguales contiene este formato? "*" cabe
    // en una racha de 1 o 3 (cursiva / negrita+cursiva) pero no en la de 2.
    auto containsFormat = [&](int run) {
        if (!repeated) return true;
        return run >= n && run <= 3 && !(n == 1 && run == 2);
    };

    // Ya envuelto por dentro de la selección: la selección incluye los marcadores.
    const QString selected = textAt(start, end - start);
    bool wrappedInside = selected.size() >= n + m && selected.startsWith(open) &&
                         selected.endsWith(close);
    if (wrappedInside && repeated) {
        wrappedInside = containsFormat(runLength(start, +1)) && containsFormat(runLength(end - 1, -1));
    }
    if (wrappedInside) {
        cursor.setPosition(end - m);
        cursor.setPosition(end, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.setPosition(start);
        cursor.setPosition(start + n, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.setPosition(start);
        cursor.setPosition(end - n - m, QTextCursor::KeepAnchor);
        return;
    }

    // Ya envuelto por fuera: los marcadores rodean la selección.
    bool wrappedAround = textAt(start - n, n) == open && textAt(end, m) == close;
    if (wrappedAround && repeated) {
        wrappedAround = containsFormat(runLength(start - 1, -1)) && containsFormat(runLength(end, +1));
    }
    if (wrappedAround) {
        cursor.setPosition(end);
        cursor.setPosition(end + m, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.setPosition(start - n);
        cursor.setPosition(start, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.setPosition(start - n);
        cursor.setPosition(end - n, QTextCursor::KeepAnchor);
        return;
    }

    cursor.setPosition(end);
    cursor.insertText(close);
    cursor.setPosition(start);
    cursor.insertText(open);
    cursor.setPosition(start + n);
    cursor.setPosition(end + n, QTextCursor::KeepAnchor);
}

// --- Eventos ----------------------------------------------------------------

void EditorWidget::keyPressEvent(QKeyEvent* event) {
    const int key = event->key();
    // Pulsar solo un modificador (Alt antes de Alt+clic, p. ej.) no debe
    // deshacer el multicursor ni tocar nada.
    if (key == Qt::Key_Shift || key == Qt::Key_Control || key == Qt::Key_Alt ||
        key == Qt::Key_AltGr || key == Qt::Key_Meta || key == Qt::Key_CapsLock) {
        QPlainTextEdit::keyPressEvent(event);
        return;
    }

    const Qt::KeyboardModifiers mods =
        event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
    const bool shift = event->modifiers().testFlag(Qt::ShiftModifier);
    const bool plain = mods == Qt::NoModifier;
    // AltGr llega como Ctrl+Alt en Windows: así se escriben { } [ ] en muchas
    // distribuciones (p. ej. la española), no es un atajo.
    const bool altGr = mods == (Qt::ControlModifier | Qt::AltModifier);
    const bool hasExtras = !extraCursors_.isEmpty();

    if (key == Qt::Key_Escape && hasExtras) {
        clearExtraCursors();
        event->accept();
        return;
    }

    if (plain && (key == Qt::Key_Tab || key == Qt::Key_Backtab)) {
        indentLines(/*outdent=*/key == Qt::Key_Backtab || shift);
        event->accept();
        return;
    }

    if (plain && (key == Qt::Key_Return || key == Qt::Key_Enter)) {
        if (shift) {
            editAllCursors([](QTextCursor& cursor) { cursor.insertBlock(); });
        } else {
            editAllCursors([this](QTextCursor& cursor) { insertNewline(cursor); });
        }
        event->accept();
        return;
    }

    if (plain && key == Qt::Key_Backspace) {
        editAllCursors([this](QTextCursor& cursor) { backspace(cursor); });
        event->accept();
        return;
    }

    if (hasExtras) {
        if (plain && key == Qt::Key_Delete) {
            editAllCursors([](QTextCursor& cursor) {
                if (cursor.hasSelection()) {
                    cursor.removeSelectedText();
                } else {
                    cursor.deleteChar();
                }
            });
            event->accept();
            return;
        }

        std::optional<QTextCursor::MoveOperation> operation;
        if (plain || mods == Qt::ControlModifier) {
            const bool word = mods == Qt::ControlModifier;
            switch (key) {
            case Qt::Key_Left:
                operation = word ? QTextCursor::PreviousWord : QTextCursor::Left;
                break;
            case Qt::Key_Right:
                operation = word ? QTextCursor::NextWord : QTextCursor::Right;
                break;
            case Qt::Key_Up:
                if (!word) operation = QTextCursor::Up;
                break;
            case Qt::Key_Down:
                if (!word) operation = QTextCursor::Down;
                break;
            case Qt::Key_Home:
                if (!word) operation = QTextCursor::StartOfLine;
                break;
            case Qt::Key_End:
                if (!word) operation = QTextCursor::EndOfLine;
                break;
            default:
                break;
            }
        }
        if (operation) {
            moveAllCursors(*operation, shift);
            event->accept();
            return;
        }
    }

    const QString text = event->text();
    if ((plain || altGr) && !overwriteMode() && text.size() == 1 && text.at(0).isPrint()) {
        editAllCursors([&](QTextCursor& cursor) { typeText(cursor, text); });
        event->accept();
        return;
    }

    // Cualquier otra tecla (atajos, Ctrl+X, PageUp...) opera solo sobre el
    // cursor principal: los extras dejarían de tener sentido, así que se quitan.
    if (hasExtras && !event->matches(QKeySequence::Paste)) clearExtraCursors();
    QPlainTextEdit::keyPressEvent(event);
}

void EditorWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (event->modifiers().testFlag(Qt::AltModifier)) {
            addExtraCursorAt(event->position().toPoint());
            event->accept();
            return;
        }
        if (!extraCursors_.isEmpty()) clearExtraCursors();
    }
    QPlainTextEdit::mousePressEvent(event);
}

void EditorWidget::wheelEvent(QWheelEvent* event) {
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        // Se acumula: los trackpads mandan muchos eventos de pocas unidades.
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
    QPlainTextEdit::wheelEvent(event);
}

void EditorWidget::paintEvent(QPaintEvent* event) {
    QPlainTextEdit::paintEvent(event);
    if (extraCursors_.isEmpty() || !hasFocus() || !extraCaretVisible_) return;

    QPainter painter(viewport());
    const QColor color = palette().color(QPalette::Text);
    for (const QTextCursor& cursor : extraCursors_) {
        QRect caret = cursorRect(cursor);
        caret.setWidth(cursorWidth());
        if (caret.intersects(event->rect())) painter.fillRect(caret, color);
    }
}

void EditorWidget::focusInEvent(QFocusEvent* event) {
    QPlainTextEdit::focusInEvent(event);
    restartCaretBlink();
}

void EditorWidget::focusOutEvent(QFocusEvent* event) {
    QPlainTextEdit::focusOutEvent(event);
    viewport()->update();
}

void EditorWidget::contextMenuEvent(QContextMenuEvent* event) {
    // Como en VS Code, el clic derecho coloca el cursor donde se hizo, salvo
    // que caiga dentro de la selección actual o haya multicursor activo.
    const QTextCursor clicked = cursorForPosition(event->pos());
    const QTextCursor current = textCursor();
    const bool insideSelection = current.hasSelection() &&
                                 clicked.position() >= current.selectionStart() &&
                                 clicked.position() <= current.selectionEnd();
    if (extraCursors_.isEmpty() && !insideSelection) setTextCursor(clicked);

    QMenu* menu = createStandardContextMenu(event->pos());
    QAction* firstStandard = menu->actions().isEmpty() ? nullptr : menu->actions().first();

    // Sugerencias ortográficas, si la palabra bajo el puntero está mal escrita.
    if (spellChecker_ && spellChecker_->isAvailable()) {
        const QTextBlock block = clicked.block();
        const QString text = block.text();
        const int column = clicked.position() - block.position();
        auto words = wordPattern().globalMatch(text);
        while (words.hasNext()) {
            const QRegularExpressionMatch word = words.next();
            if (column < word.capturedStart() || column > word.capturedEnd()) continue;
            if (spellChecker_->isCorrect(word.captured().toStdString())) break;

            const int wordStart = block.position() + static_cast<int>(word.capturedStart());
            const int wordEnd = block.position() + static_cast<int>(word.capturedEnd());
            const auto suggestions = spellChecker_->suggest(word.captured().toStdString());

            constexpr std::size_t kMaxSuggestions = 6;
            for (std::size_t i = 0; i < suggestions.size() && i < kMaxSuggestions; ++i) {
                const QString replacement = QString::fromStdString(suggestions[i]);
                auto* action = new QAction(replacement, menu);
                QFont bold = action->font();
                bold.setBold(true);
                action->setFont(bold);
                connect(action, &QAction::triggered, this, [this, wordStart, wordEnd, replacement] {
                    QTextCursor edit(document());
                    edit.setPosition(wordStart);
                    edit.setPosition(wordEnd, QTextCursor::KeepAnchor);
                    edit.insertText(replacement);
                });
                menu->insertAction(firstStandard, action);
            }
            if (suggestions.empty()) {
                auto* none = new QAction(tr("Sin sugerencias"), menu);
                none->setEnabled(false);
                menu->insertAction(firstStandard, none);
            }
            menu->insertSeparator(firstStandard);
            break;
        }
    }

    for (QAction* action : {boldAction_, italicAction_, strikeAction_, underlineAction_}) {
        menu->insertAction(firstStandard, action);
    }
    menu->insertSeparator(firstStandard);

    menu->exec(event->globalPos());
    delete menu;
}

// --- Portapapeles -------------------------------------------------------------

bool EditorWidget::canInsertFromMimeData(const QMimeData* source) const {
    return source->hasImage() || QPlainTextEdit::canInsertFromMimeData(source);
}

void EditorWidget::insertFromMimeData(const QMimeData* source) {
    if (source->hasImage() && !noteDirectory_.isEmpty()) {
        QImage image = qvariant_cast<QImage>(source->imageData());
        if (!image.isNull()) {
            QString relativePath = saveAttachment(image);
            if (!relativePath.isEmpty()) {
                insertPlainText(QString("![](%1)").arg(relativePath));
                return;
            }
        }
    }
    if (!extraCursors_.isEmpty() && source->hasText()) {
        const QString text = source->text();
        editAllCursors([&](QTextCursor& cursor) { cursor.insertText(text); });
        return;
    }
    QPlainTextEdit::insertFromMimeData(source);
}

QString EditorWidget::saveAttachment(const QImage& image) {
    QDir noteDir(noteDirectory_);
    QString attachmentsDirPath = noteDir.filePath(kAttachmentsDirName);
    if (!QDir().mkpath(attachmentsDirPath)) return QString();

    QString fileName = QUuid::createUuid().toString(QUuid::WithoutBraces) + ".png";
    QString fullPath = QDir(attachmentsDirPath).filePath(fileName);
    if (!image.save(fullPath, "PNG")) return QString();

    return QString::fromLatin1("%1/%2").arg(kAttachmentsDirName, fileName);
}

} // namespace noctis::editor
