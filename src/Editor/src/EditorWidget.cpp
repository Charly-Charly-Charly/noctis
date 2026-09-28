#include "Editor/EditorWidget.h"

#include <QDir>
#include <QImage>
#include <QMimeData>
#include <QUuid>

#include "Editor/SpellCheckHighlighter.h"

namespace noctis::editor {

namespace {
constexpr const char* kAttachmentsDirName = ".noctis-attachments";
}

EditorWidget::EditorWidget(QWidget* parent) : QPlainTextEdit(parent) {
    setLineWrapMode(QPlainTextEdit::NoWrap);
    connect(this, &QPlainTextEdit::textChanged, this, &EditorWidget::handleTextChanged);
}

EditorWidget::~EditorWidget() = default;

void EditorWidget::handleTextChanged() {
    emit contentChanged(toPlainText());
}

void EditorWidget::setWordWrapEnabled(bool enabled) {
    setLineWrapMode(enabled ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

void EditorWidget::setTabWidth(int spaces) {
    setTabStopDistance(spaces * fontMetrics().horizontalAdvance(QLatin1Char(' ')));
}

void EditorWidget::setNoteDirectory(const QString& directory) {
    noteDirectory_ = directory;
}

void EditorWidget::setSpellChecker(const spelling::SpellChecker* checker) {
    spellHighlighter_.reset();
    if (checker) {
        spellHighlighter_ = std::make_unique<SpellCheckHighlighter>(document(), *checker);
    }
}

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
