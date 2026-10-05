#include "UI/Icons.h"

#include <QEvent>
#include <QPainter>
#include <QPixmapCache>
#include <QSvgRenderer>

#include <map>
#include <memory>

namespace noctis::ui {

namespace icons {

namespace {

// Un renderer por ícono, compartido: parsear el SVG es lo más caro.
QSvgRenderer* rendererFor(const QString& name) {
    static std::map<QString, std::unique_ptr<QSvgRenderer>> renderers;
    auto it = renderers.find(name);
    if (it == renderers.end()) {
        it = renderers
                 .emplace(name,
                          std::make_unique<QSvgRenderer>(QStringLiteral(":/icons/%1.svg").arg(name)))
                 .first;
    }
    return it->second->isValid() ? it->second.get() : nullptr;
}

} // namespace

QPixmap pixmap(const QString& name, const QColor& color, int size, qreal devicePixelRatio) {
    // Los fundidos de hover piden colores intermedios en cada cuadro: la caché
    // de Qt (acotada) evita re-renderizar el SVG para cada uno.
    const QString key = QStringLiteral("noctis-icon/%1/%2/%3/%4")
                            .arg(name, color.name(QColor::HexArgb))
                            .arg(size)
                            .arg(devicePixelRatio);
    QPixmap cached;
    if (QPixmapCache::find(key, &cached)) return cached;

    QSvgRenderer* renderer = rendererFor(name);
    if (!renderer) return {};

    QPixmap result(QSize(size, size) * devicePixelRatio);
    result.setDevicePixelRatio(devicePixelRatio);
    result.fill(Qt::transparent);
    {
        QPainter painter(&result);
        painter.setRenderHint(QPainter::Antialiasing);
        renderer->render(&painter, QRectF(0, 0, size, size));
        // Los SVG vienen en negro: se conserva solo su forma (alfa) y se rellena del color pedido.
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(QRectF(0, 0, size, size), color);
    }
    QPixmapCache::insert(key, result);
    return result;
}

} // namespace icons

IconLabel::IconLabel(const QString& iconName, int iconSize, QWidget* parent)
    : QWidget(parent), iconName_(iconName), iconSize_(iconSize) {
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

QSize IconLabel::sizeHint() const {
    return {iconSize_, iconSize_};
}

void IconLabel::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    const QPixmap icon =
        icons::pixmap(iconName_, palette().color(QPalette::WindowText), iconSize_, devicePixelRatioF());
    painter.drawPixmap((width() - iconSize_) / 2, (height() - iconSize_) / 2, icon);
}

void IconLabel::changeEvent(QEvent* event) {
    // El stylesheet fija el color vía la paleta: al cambiar de tema o de
    // estado (p. ej. fila activa) hay que repintar con el color nuevo.
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::StyleChange) update();
    QWidget::changeEvent(event);
}

} // namespace noctis::ui
