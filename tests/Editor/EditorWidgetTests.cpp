#include <gtest/gtest.h>

#include <QAction>
#include <QApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTextBlock>
#include <QTextCursor>
#include <QWheelEvent>

#include "Editor/EditorWidget.h"

using noctis::editor::EditorWidget;

namespace {

// Hace falta una QApplication viva para crear widgets; "offscreen" la deja
// correr sin pantalla (ver main() al final y tests/Editor/CMakeLists.txt).
QApplication& app() {
    static int argc = 1;
    static char name[] = "noctis_editor_tests";
    static char* argv[] = {name, nullptr};
    static QApplication application(argc, argv);
    return application;
}

class EditorWidgetTest : public ::testing::Test {
protected:
    void SetUp() override {
        app();
        editor = std::make_unique<EditorWidget>();
        editor->resize(500, 300);
        editor->show();
        QApplication::processEvents();
    }

    void press(int key, Qt::KeyboardModifiers mods = Qt::NoModifier, const QString& keyText = {}) {
        QKeyEvent event(QEvent::KeyPress, key, mods, keyText);
        QApplication::sendEvent(editor.get(), &event);
    }

    void type(const QString& typed) {
        for (QChar ch : typed) {
            if (ch == QLatin1Char('\n')) {
                press(Qt::Key_Return);
            } else {
                press(ch.unicode(), Qt::NoModifier, QString(ch));
            }
        }
    }

    void setText(const QString& content, int position = -1) {
        editor->setPlainText(content);
        QTextCursor cursor = editor->textCursor();
        cursor.setPosition(position < 0 ? static_cast<int>(content.size()) : position);
        editor->setTextCursor(cursor);
    }

    void triggerFormat(const QString& name) {
        for (QAction* action : editor->actions()) {
            if (action->text() == name) {
                action->trigger();
                return;
            }
        }
        FAIL() << "acción no encontrada: " << name.toStdString();
    }

    // Alt+clic sobre la columna `column` de la línea `block`.
    void altClickAtLine(int block, int column = 0) {
        QTextCursor cursor(editor->document()->findBlockByNumber(block));
        cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, column);
        const QPoint point = editor->cursorRect(cursor).center();
        QMouseEvent event(QEvent::MouseButtonPress, point, editor->viewport()->mapToGlobal(point),
                          Qt::LeftButton, Qt::LeftButton, Qt::AltModifier);
        QApplication::sendEvent(editor->viewport(), &event);
    }

    QString text() const { return editor->toPlainText(); }

    std::unique_ptr<EditorWidget> editor;
};

TEST_F(EditorWidgetTest, ClosesPairsAndSkipsOverTheCloser) {
    type("(");
    EXPECT_EQ(text(), "()");
    EXPECT_EQ(editor->textCursor().position(), 1);
    type(")");
    EXPECT_EQ(text(), "()");
    EXPECT_EQ(editor->textCursor().position(), 2);
}

TEST_F(EditorWidgetTest, ClosesAllBracketAndQuoteKinds) {
    for (const QString& opener : {QString("["), QString("{"), QString("\""), QString("'"),
                                  QString("`")}) {
        editor->clear();
        type(opener);
        EXPECT_EQ(text().size(), 2) << opener.toStdString();
        EXPECT_EQ(editor->textCursor().position(), 1) << opener.toStdString();
    }
}

TEST_F(EditorWidgetTest, DoesNotPairBeforeAWordOrAfterALetterForQuotes) {
    setText("hola", 0);
    type("(");
    EXPECT_EQ(text(), "(hola"); // delante de una palabra: no se cierra

    setText("it", 2);
    type("'");
    EXPECT_EQ(text(), "it'"); // apóstrofe tras una letra
}

TEST_F(EditorWidgetTest, TripleBacktickDoesNotMultiply) {
    type("```");
    EXPECT_EQ(text(), "```");
}

TEST_F(EditorWidgetTest, BackspaceBetweenEmptyPairRemovesBoth) {
    type("(");
    press(Qt::Key_Backspace);
    EXPECT_EQ(text(), "");
}

TEST_F(EditorWidgetTest, TypingAnOpenerOverASelectionWrapsIt) {
    setText("hola");
    editor->selectAll();
    type("(");
    EXPECT_EQ(text(), "(hola)");
    EXPECT_EQ(editor->textCursor().selectedText(), "hola");

    type("*");
    EXPECT_EQ(text(), "(*hola*)");
}

TEST_F(EditorWidgetTest, EnterContinuesBulletList) {
    setText("- uno");
    press(Qt::Key_Return);
    EXPECT_EQ(text(), "- uno\n- ");
}

TEST_F(EditorWidgetTest, EnterOnEmptyBulletEndsTheList) {
    setText("- uno\n- ");
    press(Qt::Key_Return);
    EXPECT_EQ(text(), "- uno\n");
}

TEST_F(EditorWidgetTest, EnterIncrementsNumberedListAndKeepsIndent) {
    setText("  3. tres");
    press(Qt::Key_Return);
    EXPECT_EQ(text(), "  3. tres\n  4. ");
}

TEST_F(EditorWidgetTest, EnterContinuesChecklistUnchecked) {
    setText("- [x] hecho");
    press(Qt::Key_Return);
    EXPECT_EQ(text(), "- [x] hecho\n- [ ] ");
}

TEST_F(EditorWidgetTest, EnterSplitsListItemInTheMiddle) {
    setText("- hola mundo", 6);
    press(Qt::Key_Return);
    EXPECT_EQ(text(), "- hola\n-  mundo");
}

TEST_F(EditorWidgetTest, EnterInsideCodeFenceIsPlain) {
    setText("```\n- x");
    press(Qt::Key_Return);
    EXPECT_EQ(text(), "```\n- x\n");
}

TEST_F(EditorWidgetTest, BackspaceOnEmptyBulletRemovesMarker) {
    setText("- ");
    press(Qt::Key_Backspace);
    EXPECT_EQ(text(), "");
}

TEST_F(EditorWidgetTest, TabIndentsEverySelectedLine) {
    setText("a\nb\nc");
    editor->selectAll();
    press(Qt::Key_Tab);
    EXPECT_EQ(text(), "\ta\n\tb\n\tc");
    // La selección sigue abarcando todo, incluida la sangría de la primera línea.
    EXPECT_EQ(editor->textCursor().selectionStart(), 0);
    EXPECT_EQ(editor->textCursor().selectionEnd(), static_cast<int>(text().size()));
}

TEST_F(EditorWidgetTest, ShiftTabOutdentsEverySelectedLine) {
    setText("\ta\n\tb\n    c");
    editor->selectAll();
    press(Qt::Key_Backtab, Qt::ShiftModifier);
    EXPECT_EQ(text(), "a\nb\nc");
}

TEST_F(EditorWidgetTest, TabWithoutSelectionInsertsTab) {
    setText("ab", 1);
    press(Qt::Key_Tab);
    EXPECT_EQ(text(), "a\tb");
}

TEST_F(EditorWidgetTest, TabOnListLineNestsTheItem) {
    setText("- uno");
    press(Qt::Key_Tab);
    EXPECT_EQ(text(), "\t- uno");
    press(Qt::Key_Backtab, Qt::ShiftModifier);
    EXPECT_EQ(text(), "- uno");
}

TEST_F(EditorWidgetTest, SelectionEndingAtLineStartDoesNotIndentThatLine) {
    setText("a\nb\nc");
    QTextCursor cursor = editor->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(4, QTextCursor::KeepAnchor); // hasta el inicio de "c"
    editor->setTextCursor(cursor);
    press(Qt::Key_Tab);
    EXPECT_EQ(text(), "\ta\n\tb\nc");
}

TEST_F(EditorWidgetTest, AltClickAddsCursorsAndTypingAppliesToAll) {
    setText("a\nb\nc", 0);
    altClickAtLine(1);
    altClickAtLine(2);
    type("X");
    EXPECT_EQ(text(), "Xa\nXb\nXc");

    press(Qt::Key_Backspace);
    EXPECT_EQ(text(), "a\nb\nc");
}

TEST_F(EditorWidgetTest, AltClickOnExistingExtraCursorRemovesIt) {
    setText("a\nb", 0);
    altClickAtLine(1);
    altClickAtLine(1);
    type("X");
    EXPECT_EQ(text(), "Xa\nb");
}

TEST_F(EditorWidgetTest, EscapeLeavesOnlyThePrimaryCursor) {
    setText("a\nb", 0);
    altClickAtLine(1);
    press(Qt::Key_Escape);
    type("X");
    EXPECT_EQ(text(), "Xa\nb");
}

TEST_F(EditorWidgetTest, PlainClickClearsExtraCursors) {
    setText("a\nb", 0);
    altClickAtLine(1);
    const QPoint point = editor->cursorRect(editor->textCursor()).center();
    QMouseEvent click(QEvent::MouseButtonPress, point, editor->viewport()->mapToGlobal(point),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(editor->viewport(), &click);
    type("X");
    EXPECT_EQ(text(), "Xa\nb");
}

TEST_F(EditorWidgetTest, MultiCursorEditIsASingleUndoStep) {
    setText("a\nb", 1);
    altClickAtLine(1, 1);
    type("(");
    EXPECT_EQ(text(), "a()\nb()");
    editor->undo();
    EXPECT_EQ(text(), "a\nb");
}

TEST_F(EditorWidgetTest, ArrowKeysMoveEveryCursor) {
    setText("ab\ncd", 0);
    altClickAtLine(1);
    press(Qt::Key_Right);
    type("X");
    EXPECT_EQ(text(), "aXb\ncXd");
}

TEST_F(EditorWidgetTest, FormatActionsToggleBoldOnTheWordUnderCursor) {
    setText("hola mundo", 2);
    triggerFormat("Negrita");
    EXPECT_EQ(text(), "**hola** mundo");
    triggerFormat("Negrita");
    EXPECT_EQ(text(), "hola mundo");
}

TEST_F(EditorWidgetTest, ItalicAndBoldDoNotConfuseTheirMarkers) {
    setText("hola");
    editor->selectAll();
    triggerFormat("Negrita");
    EXPECT_EQ(text(), "**hola**");
    triggerFormat("Cursiva");
    EXPECT_EQ(text(), "***hola***");
    triggerFormat("Negrita");
    EXPECT_EQ(text(), "*hola*");
    triggerFormat("Cursiva");
    EXPECT_EQ(text(), "hola");
}

TEST_F(EditorWidgetTest, StrikeAndUnderline) {
    setText("hola");
    editor->selectAll();
    triggerFormat("Tachado");
    EXPECT_EQ(text(), "~~hola~~");
    triggerFormat("Tachado");
    EXPECT_EQ(text(), "hola");
    triggerFormat("Subrayado");
    EXPECT_EQ(text(), "<u>hola</u>");
    triggerFormat("Subrayado");
    EXPECT_EQ(text(), "hola");
}

TEST_F(EditorWidgetTest, FormatWithoutWordInsertsEmptyPair) {
    setText("");
    triggerFormat("Negrita");
    EXPECT_EQ(text(), "****");
    EXPECT_EQ(editor->textCursor().position(), 2);
}

TEST_F(EditorWidgetTest, FormatAppliesToEveryCursor) {
    setText("uno\ndos", 1);
    altClickAtLine(1, 1);
    triggerFormat("Cursiva");
    EXPECT_EQ(text(), "*uno*\n*dos*");
}

TEST_F(EditorWidgetTest, ZoomLevelChangesFontSize) {
    const qreal base = editor->font().pointSizeF();
    editor->setZoomLevel(3);
    EXPECT_DOUBLE_EQ(editor->font().pointSizeF(), base + 3);
    editor->setZoomLevel(0);
    EXPECT_DOUBLE_EQ(editor->font().pointSizeF(), base);
}

TEST_F(EditorWidgetTest, CtrlWheelRequestsZoomSteps) {
    int steps = 0;
    QObject::connect(editor.get(), &EditorWidget::zoomStepRequested, editor.get(),
                     [&steps](int direction) { steps += direction; });
    const QPoint point(10, 10);
    QWheelEvent up(point, editor->viewport()->mapToGlobal(point), QPoint(), QPoint(0, 120),
                   Qt::NoButton, Qt::ControlModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(editor->viewport(), &up);
    QWheelEvent down(point, editor->viewport()->mapToGlobal(point), QPoint(), QPoint(0, -240),
                     Qt::NoButton, Qt::ControlModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(editor->viewport(), &down);
    EXPECT_EQ(steps, -1);
}

TEST_F(EditorWidgetTest, FindSelectsFirstMatchFromCursorAndReportsCounts) {
    int current = -1, total = -1;
    QObject::connect(editor.get(), &EditorWidget::findResultChanged, editor.get(),
                     [&](int c, int t) { current = c; total = t; });

    setText("Casa casa CASA perro", 5); // cursor entre la 1.ª y la 2.ª
    editor->setFindQuery("casa");
    EXPECT_EQ(total, 3); // sin distinguir mayúsculas
    EXPECT_EQ(current, 2);
    EXPECT_EQ(editor->textCursor().selectedText(), "casa");
    EXPECT_EQ(editor->textCursor().selectionStart(), 5);
}

TEST_F(EditorWidgetTest, FindNextAndPreviousWrapAround) {
    setText("a b a b a", 0);
    editor->setFindQuery("a");
    EXPECT_EQ(editor->textCursor().selectionStart(), 0);
    editor->findNext();
    EXPECT_EQ(editor->textCursor().selectionStart(), 4);
    editor->findNext();
    EXPECT_EQ(editor->textCursor().selectionStart(), 8);
    editor->findNext(); // da la vuelta
    EXPECT_EQ(editor->textCursor().selectionStart(), 0);
    editor->findNext(/*backwards=*/true); // y hacia atrás también
    EXPECT_EQ(editor->textCursor().selectionStart(), 8);
}

TEST_F(EditorWidgetTest, FindWithNoMatchesAndClear) {
    int total = -1;
    QObject::connect(editor.get(), &EditorWidget::findResultChanged, editor.get(),
                     [&](int, int t) { total = t; });
    setText("hola");
    editor->setFindQuery("zzz");
    EXPECT_EQ(total, 0);
    editor->setFindQuery("hola");
    EXPECT_EQ(total, 1);
    editor->clearFind();
    EXPECT_EQ(total, 0);
    editor->findNext(); // sin búsqueda activa no hace nada ni falla
}

TEST_F(EditorWidgetTest, FindResultsFollowEdits) {
    int total = -1;
    QObject::connect(editor.get(), &EditorWidget::findResultChanged, editor.get(),
                     [&](int, int t) { total = t; });
    setText("x x", 0);
    editor->setFindQuery("x");
    EXPECT_EQ(total, 2);
    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    editor->setTextCursor(cursor);
    type(" x");
    EXPECT_EQ(total, 3);
}

} // namespace

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
