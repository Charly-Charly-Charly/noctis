#include <gtest/gtest.h>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QPdfWriter>
#include <QTemporaryDir>
#include <QTextDocument>

#include <memory>

#include "UI/PreviewWidget.h"

using noctis::ui::PreviewWidget;

// Ancho de una página A4 con márgenes de 10 mm, en píxeles lógicos de QTextDocument.
constexpr qreal kPageWidth = 718;

namespace {

QApplication& app() {
    static int argc = 1;
    static char name[] = "noctis_ui_tests";
    static char* argv[] = {name, nullptr};
    static QApplication application(argc, argv);
    return application;
}

class PreviewPrintTest : public ::testing::Test {
protected:
    void SetUp() override {
        app();
        ASSERT_TRUE(dir.isValid());
        QDir(dir.path()).mkpath(".noctis-attachments");
        QImage small(100, 50, QImage::Format_RGB32);
        small.fill(Qt::red);
        ASSERT_TRUE(small.save(dir.filePath(".noctis-attachments/small.png")));
        QImage wide(2000, 400, QImage::Format_RGB32);
        wide.fill(Qt::blue);
        ASSERT_TRUE(wide.save(dir.filePath(".noctis-attachments/wide.png")));
        preview = std::make_unique<PreviewWidget>();
        preview->setBaseDirectory(dir.path());
    }

    // Imprime el documento a un PDF real y devuelve sus bytes.
    QByteArray printToPdf(QTextDocument& document) {
        const QString path = dir.filePath("out.pdf");
        {
            QPdfWriter writer(path);
            writer.setResolution(300);
            document.print(&writer);
        }
        QFile file(path);
        EXPECT_TRUE(file.open(QIODevice::ReadOnly));
        return file.readAll();
    }

    QTemporaryDir dir;
    // Se crea en SetUp: un QWidget necesita la QApplication ya viva.
    std::unique_ptr<PreviewWidget> preview;
};

TEST_F(PreviewPrintTest, RelativeImagesAreEmbeddedInThePdf) {
    auto withImage = preview->createPrintDocument("hola\n\n![](.noctis-attachments/small.png)\n");
    auto withoutImage = preview->createPrintDocument("hola\n");
    const QByteArray pdfWith = printToPdf(*withImage);
    const QByteArray pdfWithout = printToPdf(*withoutImage);

    EXPECT_TRUE(pdfWith.contains("/Subtype /Image"));
    EXPECT_FALSE(pdfWithout.contains("/Subtype /Image"));
}

TEST_F(PreviewPrintTest, MissingImageDoesNotBreakExport) {
    auto document = preview->createPrintDocument("![](.noctis-attachments/nope.png)\n");
    EXPECT_FALSE(printToPdf(*document).isEmpty());
}

TEST_F(PreviewPrintTest, ImagesWiderThanThePageAreScaledDown) {
    auto document = preview->createPrintDocument("![](.noctis-attachments/wide.png)\n");
    document->setPageSize(QSizeF(kPageWidth, 5000));
    EXPECT_LE(document->idealWidth(), kPageWidth);
}

TEST_F(PreviewPrintTest, ImagePathsWithSpacesResolve) {
    QImage image(10, 10, QImage::Format_RGB32);
    image.fill(Qt::green);
    ASSERT_TRUE(image.save(dir.filePath(".noctis-attachments/con espacio.png")));
    auto document = preview->createPrintDocument("![](.noctis-attachments/con%20espacio.png)\n");
    EXPECT_TRUE(printToPdf(*document).contains("/Subtype /Image"));
}

TEST_F(PreviewPrintTest, CodeBlocksWrapInsteadOfOverflowingThePage) {
    const QString longLine = QString("palabra ").repeated(60);
    // Cuatro espacios de sangría: para Markdown es un bloque de código.
    auto indented = preview->createPrintDocument("    " + longLine + "\n");
    indented->setPageSize(QSizeF(kPageWidth, 5000));
    EXPECT_LE(indented->idealWidth(), kPageWidth);

    auto fenced = preview->createPrintDocument("```\n" + longLine + "\n```\n");
    fenced->setPageSize(QSizeF(kPageWidth, 5000));
    EXPECT_LE(fenced->idealWidth(), kPageWidth);
}

TEST_F(PreviewPrintTest, DeeplyNestedTabIndentedListsWrap) {
    const QString longText = QString("texto largo ").repeated(40);
    const QString markdown = "- uno\n\t- dos\n\t\t- tres\n\t\t\t- " + longText + "\n";
    auto document = preview->createPrintDocument(markdown);
    document->setPageSize(QSizeF(kPageWidth, 5000));
    EXPECT_LE(document->idealWidth(), kPageWidth);
}

} // namespace

int main(int argc, char** argv) {
    qputenv("QT_QPA_PLATFORM", "offscreen");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
