#include "Editor/SpellCheckHighlighter.h"

#include <QRegularExpression>
#include <QTextCharFormat>

#include "Spelling/SpellChecker.h"

namespace noctis::editor {

namespace {
// Letras latinas más acentos/diéresis/ñ propios del español: separa el
// bloque en "palabras" para chequear cada una por su cuenta.
const QRegularExpression& wordPattern() {
    static const QRegularExpression pattern(QStringLiteral("[A-Za-zÀ-ÖØ-öø-ÿ]+"));
    return pattern;
}
} // namespace

SpellCheckHighlighter::SpellCheckHighlighter(QTextDocument* document,
                                              const spelling::SpellChecker& checker)
    : QSyntaxHighlighter(document), checker_(checker) {}

void SpellCheckHighlighter::highlightBlock(const QString& text) {
    if (!checker_.isAvailable()) return;

    QTextCharFormat misspelledFormat;
    misspelledFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    misspelledFormat.setUnderlineColor(Qt::red);

    auto matches = wordPattern().globalMatch(text);
    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        if (!checker_.isCorrect(match.captured().toStdString())) {
            setFormat(match.capturedStart(), match.capturedLength(), misspelledFormat);
        }
    }
}

} // namespace noctis::editor
