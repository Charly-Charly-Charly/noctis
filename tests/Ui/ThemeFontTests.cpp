#include <gtest/gtest.h>

#include <QFont>
#include <QRegularExpression>

#include "UI/Theme.h"

using namespace noctis::ui;

// Regresión de memoria: una lista de familias de respaldo (QFont::setFamilies o
// "font-family: A, B" en el QSS) le cuesta a Qt ~30 MB de RAM (se midió pasar de
// ~36 a ~68 MB). La app debe usar siempre UNA sola familia.

TEST(ThemeFont, ApplicationFontHasASingleFamily) {
    const QFont font = theme::applicationFont();
    EXPECT_FALSE(theme::monospaceFamily().isEmpty());
    EXPECT_EQ(font.family(), theme::monospaceFamily());
    EXPECT_LE(font.families().size(), 1);
}

TEST(ThemeFont, StylesheetsDeclareASingleFontFamily) {
    for (bool dark : {false, true}) {
        const QString css = theme::stylesheet(dark);
        const QRegularExpression families(R"(font-family:\s*([^;]*);)");
        auto matches = families.globalMatch(css);
        int found = 0;
        while (matches.hasNext()) {
            ++found;
            EXPECT_FALSE(matches.next().captured(1).contains(','))
                << "font-family con lista de familias en el QSS";
        }
        EXPECT_GE(found, 1);
    }
}
