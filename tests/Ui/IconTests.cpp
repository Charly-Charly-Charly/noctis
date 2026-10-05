#include <gtest/gtest.h>

#include <QApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QVBoxLayout>

#include "UI/IconButton.h"
#include "UI/Icons.h"
#include "UI/Theme.h"
#include "UI/TreeRowWidget.h"

using namespace noctis::ui;

namespace {

const char* const kIconNames[] = {"add",   "collapse",   "expand",  "settings", "keyboard",
                                  "help",  "arrow_back", "more_vert", "arrow_up", "arrow_down",
                                  "close", "star",       "history", "folder"};

// ¿Hay algún píxel con trazo (no transparente)?
bool hasInk(const QImage& image) {
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 0) return true;
        }
    }
    return false;
}

TEST(Icons, EveryBundledIconRendersInTheRequestedColor) {
    const QColor color(200, 30, 60);
    for (const char* name : kIconNames) {
        const QPixmap pixmap = icons::pixmap(name, color, 16);
        ASSERT_FALSE(pixmap.isNull()) << name;
        const QImage image = pixmap.toImage().convertToFormat(QImage::Format_ARGB32);
        ASSERT_TRUE(hasInk(image)) << name;
        // Todo píxel con trazo es del color pedido (±3 por el redondeo del alfa premultiplicado).
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                const QRgb px = image.pixel(x, y);
                if (qAlpha(px) < 200) continue;
                EXPECT_NEAR(qRed(px), 200, 3) << name;
                EXPECT_NEAR(qGreen(px), 30, 3) << name;
                EXPECT_NEAR(qBlue(px), 60, 3) << name;
            }
        }
    }
}

TEST(Icons, UnknownIconGivesEmptyPixmapInsteadOfCrashing) {
    EXPECT_TRUE(icons::pixmap("no_existe", Qt::black, 16).isNull());
}

TEST(Icons, HiDpiPixmapIsLargerButKeepsLogicalSize) {
    const QPixmap pixmap = icons::pixmap("add", Qt::black, 16, 2.0);
    EXPECT_EQ(pixmap.width(), 32);
    EXPECT_DOUBLE_EQ(pixmap.devicePixelRatio(), 2.0);
}

TEST(IconButtonIcons, DrawsTheIconInsteadOfText) {
    IconButton button{QString()};
    button.setIconName("settings");
    button.setFixedSize(28, 28);
    button.setStyleSheet("QPushButton { background: white; }");
    const QImage image = button.grab().toImage();
    // Fondo blanco con un ícono encima: tiene que haber píxeles que no son blancos.
    bool nonWhite = false;
    for (int y = 0; y < image.height() && !nonWhite; ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y) != QColor(Qt::white)) {
                nonWhite = true;
                break;
            }
        }
    }
    EXPECT_TRUE(nonWhite);
}

// Con NOCTIS_DUMP_DIR=<carpeta> deja una captura de los íconos con el tema
// real (claro y oscuro) para revisarlos a ojo; sin la variable no hace nada.
TEST(IconButtonIcons, DumpGalleryForVisualReview) {
    const QString dir = qEnvironmentVariable("NOCTIS_DUMP_DIR");
    if (dir.isEmpty()) GTEST_SKIP() << "NOCTIS_DUMP_DIR no definido";

    for (bool dark : {false, true}) {
        qApp->setStyleSheet(theme::stylesheet(dark));
        QWidget gallery;
        gallery.setObjectName("sidebar");
        auto* layout = new QHBoxLayout(&gallery);
        layout->setSpacing(8);
        for (const char* name : kIconNames) {
            auto* button = makeIconButton(name, &gallery);
            button->setObjectName("sidebarIconButton");
            button->setFixedSize(28, 28);
            layout->addWidget(button);
        }
        auto* star = new IconLabel("star", 12, &gallery);
        star->setObjectName("sidebarTitleStar");
        layout->addWidget(star);
        // Filas de sección del árbol: normal y activa (ícono con color propio).
        auto* column = new QVBoxLayout();
        auto* rowNormal = new TreeRowWidget("001", "star", "Favoritos", TreeRowWidget::Kind::Section, &gallery);
        auto* rowActive = new TreeRowWidget("002", "history", "Recientes", TreeRowWidget::Kind::Section, &gallery);
        rowActive->setActive(true);
        column->addWidget(rowNormal);
        column->addWidget(rowActive);
        layout->addLayout(column);
        gallery.setStyleSheet(QString()); // el global ya aplica
        gallery.ensurePolished();
        gallery.show();
        QApplication::processEvents();
        gallery.grab().save(QDir(dir).filePath(dark ? "icons-dark.png" : "icons-light.png"));
    }
}

} // namespace
