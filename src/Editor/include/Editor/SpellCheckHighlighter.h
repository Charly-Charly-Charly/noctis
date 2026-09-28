#pragma once

#include <QSyntaxHighlighter>

namespace noctis::spelling {
class SpellChecker;
}

namespace noctis::editor {

// Subraya en rojo ondulado las palabras ausentes del diccionario cargado.
// Si el diccionario no está disponible (ver SpellChecker::isAvailable) no
// subraya nada, en vez de fallar: v1 revisa palabra por palabra sin excluir
// bloques de código ni sintaxis Markdown, así que puede marcar identificadores
// o texto de enlaces como error — limitación conocida, no un bug.
class SpellCheckHighlighter : public QSyntaxHighlighter {
public:
    SpellCheckHighlighter(QTextDocument* document, const spelling::SpellChecker& checker);

protected:
    void highlightBlock(const QString& text) override;

private:
    const spelling::SpellChecker& checker_;
};

} // namespace noctis::editor
