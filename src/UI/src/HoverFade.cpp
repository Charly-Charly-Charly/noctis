#include "UI/HoverFade.h"

#include <QAction>
#include <QApplication>
#include <QCursor>
#include <QEasingCurve>
#include <QEvent>
#include <QHash>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStyleOptionSlider>
#include <QStyledItemDelegate>
#include <QTabBar>
#include <QTimer>

#include <functional>

#include "UI/IconButton.h"

namespace noctis::ui {

namespace {
constexpr int kFadeDurationMs = 140;
constexpr int kTickMs = 15;

// Lo fija MainWindow al cambiar de tema (ver toggleDarkMode); acá se lee en
// cada pintado para que el fundido siga el tema sin recibirlo por parámetro.
QColor currentHoverColor() {
    const QVariant stored = qApp->property("noctisHoverColor");
    return stored.isValid() ? stored.value<QColor>() : QColor(0xB9, 0xBB, 0xB0);
}

// Progreso de hover por "fila" (ítem de lista, de menú, pestaña...). Cada
// tick consulta qué fila está bajo el mouse y acerca el progreso de todas
// hacia su objetivo (1 la actual, 0 las demás), así los eventos solo tienen
// que avisar "puede haber cambiado" con poke().
class RowFader : public QObject {
public:
    RowFader(std::function<int()> currentKey, std::function<void()> onChange, QObject* parent)
        : QObject(parent), currentKey_(std::move(currentKey)), onChange_(std::move(onChange)) {
        timer_.setInterval(kTickMs);
        connect(&timer_, &QTimer::timeout, this, [this] { step(); });
    }

    void poke() {
        if (!timer_.isActive()) timer_.start();
    }

    void reset() {
        progress_.clear();
        timer_.stop();
        onChange_();
    }

    qreal progress(int key) const {
        const auto it = progress_.constFind(key);
        if (it == progress_.constEnd()) return 0.0;
        const qreal t = it.value();
        return t * t * (3.0 - 2.0 * t); // smoothstep
    }

private:
    void step() {
        const int current = currentKey_();
        if (current >= 0 && !progress_.contains(current)) progress_.insert(current, 0.0);

        const qreal delta = static_cast<qreal>(kTickMs) / kFadeDurationMs;
        bool settled = true;

        for (auto it = progress_.begin(); it != progress_.end();) {
            const qreal target = it.key() == current ? 1.0 : 0.0;
            qreal value = it.value();
            value = value < target ? qMin(target, value + delta) : qMax(target, value - delta);

            if (value <= 0.0 && target == 0.0) {
                it = progress_.erase(it);
                continue;
            }
            it.value() = value;
            if (value != target) settled = false;
            ++it;
        }

        if (settled) timer_.stop();
        onChange_();
    }

    std::function<int()> currentKey_;
    std::function<void()> onChange_;
    QTimer timer_;
    QHash<int, qreal> progress_;
};

QStyleOptionButton optionFor(const QPushButton* button) {
    QStyleOptionButton option;
    option.initFrom(button);
    if (button->isDown()) option.state |= QStyle::State_Sunken;
    if (button->isChecked()) option.state |= QStyle::State_On;
    if (!button->isFlat() && !(option.state & QStyle::State_Sunken)) {
        option.state |= QStyle::State_Raised;
    }
    option.text = button->text();
    option.icon = button->icon();
    option.iconSize = button->iconSize();
    if (button->isFlat()) option.features |= QStyleOptionButton::Flat;
    if (button->isDefault()) option.features |= QStyleOptionButton::DefaultButton;
    if (button->autoDefault()) option.features |= QStyleOptionButton::AutoDefaultButton;
    if (button->menu()) option.features |= QStyleOptionButton::HasMenu;
    return option;
}

// Fila de QListWidget con resaltado de hover que aparece y se va con fundido.
class FadeHoverDelegate : public QStyledItemDelegate {
public:
    explicit FadeHoverDelegate(QListWidget* list)
        : QStyledItemDelegate(list),
          list_(list),
          fader_([this] { return hoveredRow_; }, [list] { list->viewport()->update(); }, this) {
        list_->viewport()->setMouseTracking(true);
        list_->viewport()->installEventFilter(this);

        // clear() y refresh() recrean las filas: el progreso por número de
        // fila quedaría apuntando a otra cosa.
        connect(list_->model(), &QAbstractItemModel::modelReset, this, [this] {
            hoveredRow_ = -1;
            fader_.reset();
        });
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        const qreal t = fader_.progress(index.row());
        if (t > 0.0 && !(option.state & QStyle::State_Selected)) {
            QColor tint = currentHoverColor();
            tint.setAlphaF(tint.alphaF() * t);

            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(tint);
            painter->drawRoundedRect(QRectF(option.rect), 6.0, 6.0);
            painter->restore();
        }
        QStyledItemDelegate::paint(painter, option, index);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == list_->viewport()) {
            if (event->type() == QEvent::MouseMove) {
                const auto* mouse = static_cast<QMouseEvent*>(event);
                hoveredRow_ = list_->indexAt(mouse->position().toPoint()).row();
                fader_.poke();
            } else if (event->type() == QEvent::Leave) {
                hoveredRow_ = -1;
                fader_.poke();
            }
        }
        return QStyledItemDelegate::eventFilter(watched, event);
    }

private:
    QListWidget* list_;
    RowFader fader_;
    int hoveredRow_ = -1;
};

// Resaltado de la acción activa de un QMenu o QMenuBar, con fundido. QSS no
// puede interpolar el cambio de ":selected", así que el stylesheet deja ese
// estado transparente y el resaltado se pinta acá, debajo de los ítems (el
// fondo del menú ya se pintó antes del evento de pintado y los ítems se
// pintan después, encima).
template <typename Bar>
class ActionHoverFade : public QObject {
public:
    explicit ActionHoverFade(Bar* bar)
        : QObject(bar),
          bar_(bar),
          fader_([this] { return bar_->actions().indexOf(bar_->activeAction()); },
                 [bar] { bar->update(); }, this) {
        bar_->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched != bar_) return false;

        switch (event->type()) {
            case QEvent::MouseMove:
            case QEvent::Enter:
            case QEvent::Leave:
            case QEvent::KeyPress:
            case QEvent::Show:
            case QEvent::HoverMove:
                fader_.poke();
                break;
            case QEvent::Hide:
                fader_.reset();
                break;
            case QEvent::Paint:
                paintUnderlay();
                break;
            default:
                break;
        }
        return false;
    }

private:
    void paintUnderlay() {
        const QList<QAction*> actions = bar_->actions();
        QPainter painter(bar_);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);

        for (int i = 0; i < actions.size(); ++i) {
            const qreal t = fader_.progress(i);
            QAction* action = actions[i];
            if (t <= 0.0 || action->isSeparator() || !action->isEnabled()) continue;

            QColor tint = currentHoverColor();
            tint.setAlphaF(tint.alphaF() * t);
            painter.setBrush(tint);
            painter.drawRoundedRect(QRectF(bar_->actionGeometry(action)), 6.0, 6.0);
        }
    }

    Bar* bar_;
    RowFader fader_;
};

// Hover de las pestañas: fondo que aparece con fundido bajo la pestaña (el
// color del texto ya no cambia al pasar el mouse, QSS no podría interpolarlo).
class TabBarHoverFade : public QObject {
public:
    explicit TabBarHoverFade(QTabBar* bar)
        : QObject(bar),
          bar_(bar),
          fader_([this] { return hoveredTab(); }, [bar] { bar->update(); }, this) {
        bar_->setMouseTracking(true);
        bar_->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched != bar_) return false;

        switch (event->type()) {
            case QEvent::MouseMove:
            case QEvent::Enter:
            case QEvent::Leave:
            case QEvent::HoverMove:
                fader_.poke();
                break;
            case QEvent::Paint:
                paintUnderlay();
                break;
            default:
                break;
        }
        return false;
    }

private:
    int hoveredTab() const {
        if (!bar_->underMouse()) return -1;
        return bar_->tabAt(bar_->mapFromGlobal(QCursor::pos()));
    }

    void paintUnderlay() {
        QPainter painter(bar_);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);

        for (int i = 0; i < bar_->count(); ++i) {
            const qreal t = fader_.progress(i);
            if (t <= 0.0 || i == bar_->currentIndex()) continue; // la activa ya tiene su fondo

            QColor tint = currentHoverColor();
            tint.setAlphaF(tint.alphaF() * t);
            painter.setBrush(tint);

            // Mismas esquinas superiores redondeadas que la pestaña activa
            // (ver Theme.cpp): se dibuja un rectángulo más alto y se recorta.
            const QRect rect = bar_->tabRect(i).adjusted(0, 0, -2, -2);
            painter.save();
            painter.setClipRect(rect);
            painter.drawRoundedRect(QRectF(rect.adjusted(0, 0, 0, 12)), 8.0, 8.0);
            painter.restore();
        }
    }

    QTabBar* bar_;
    RowFader fader_;
};

// Cross-fade del handle de un QScrollBar entre su estado normal y el
// ":hover" del stylesheet. Repite lo que hace QScrollBar::paintEvent (que
// usa initStyleOption, protegido) pintando dos veces con distinto estado.
class ScrollBarHoverFade : public QObject {
public:
    explicit ScrollBarHoverFade(QScrollBar* bar)
        : QObject(bar),
          bar_(bar),
          fader_([this] { return overHandle() ? 0 : -1; }, [bar] { bar->update(); }, this) {
        bar_->setAttribute(Qt::WA_Hover);
        bar_->setMouseTracking(true);
        bar_->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched != bar_) return false;

        switch (event->type()) {
            case QEvent::MouseMove:
            case QEvent::Enter:
            case QEvent::Leave:
            case QEvent::HoverEnter:
            case QEvent::HoverMove:
            case QEvent::HoverLeave:
                fader_.poke();
                return false;
            case QEvent::Paint:
                return paint();
            default:
                return false;
        }
    }

private:
    QStyleOptionSlider baseOption() const {
        QStyleOptionSlider option;
        option.initFrom(bar_);
        option.state &= ~QStyle::State_MouseOver;
        option.subControls = QStyle::SC_All;
        option.activeSubControls = QStyle::SC_None;
        option.orientation = bar_->orientation();
        option.minimum = bar_->minimum();
        option.maximum = bar_->maximum();
        option.sliderPosition = bar_->sliderPosition();
        option.sliderValue = bar_->value();
        option.singleStep = bar_->singleStep();
        option.pageStep = bar_->pageStep();
        option.upsideDown = bar_->invertedAppearance();
        if (bar_->orientation() == Qt::Horizontal) option.state |= QStyle::State_Horizontal;
        return option;
    }

    bool overHandle() const {
        if (!bar_->underMouse()) return false;
        const QStyleOptionSlider option = baseOption();
        return bar_->style()->hitTestComplexControl(QStyle::CC_ScrollBar, &option,
                                                    bar_->mapFromGlobal(QCursor::pos()),
                                                    bar_) == QStyle::SC_ScrollBarSlider;
    }

    bool paint() {
        QStyleOptionSlider option = baseOption();
        const bool dragging = bar_->isSliderDown();
        if (dragging) {
            option.activeSubControls = QStyle::SC_ScrollBarSlider;
            option.state |= QStyle::State_Sunken;
        }

        QPainter painter(bar_);
        QStyle* style = bar_->style();
        style->drawComplexControl(QStyle::CC_ScrollBar, &option, &painter, bar_);

        const qreal t = dragging ? 1.0 : fader_.progress(0);
        if (t > 0.0) {
            QStyleOptionSlider hovered = option;
            hovered.activeSubControls = QStyle::SC_ScrollBarSlider;
            hovered.state |= QStyle::State_MouseOver;
            painter.setOpacity(t);
            style->drawComplexControl(QStyle::CC_ScrollBar, &hovered, &painter, bar_);
        }
        return true;
    }

    QScrollBar* bar_;
    RowFader fader_;
};
} // namespace

ButtonHoverFade::ButtonHoverFade(QPushButton* button) : QObject(button), button_(button) {
    animation_ = new QPropertyAnimation(this, "progress", this);
    animation_->setDuration(kFadeDurationMs);
    animation_->setEasingCurve(QEasingCurve::OutCubic);

    button_->installEventFilter(this);
}

void ButtonHoverFade::setProgress(qreal progress) {
    progress_ = progress;
    button_->update();
}

void ButtonHoverFade::animateTo(qreal target) {
    animation_->stop();
    animation_->setStartValue(progress_);
    animation_->setEndValue(target);
    animation_->start();
}

bool ButtonHoverFade::eventFilter(QObject* watched, QEvent* event) {
    if (watched != button_) return false;

    switch (event->type()) {
        case QEvent::Enter:
            animateTo(1.0);
            return false;
        case QEvent::Leave:
            animateTo(0.0);
            return false;
        case QEvent::Paint:
            return paint();
        default:
            return false;
    }
}

bool ButtonHoverFade::paint() {
    QStyleOptionButton base = optionFor(button_);
    base.state &= ~QStyle::State_MouseOver;

    QPainter painter(button_);
    QStyle* style = button_->style();
    style->drawControl(QStyle::CE_PushButton, &base, &painter, button_);

    // Apretado se ve igual que con el mouse encima, sin esperar al fundido.
    const qreal t = button_->isDown() ? 1.0 : progress_;
    if (t > 0.0 && button_->isEnabled()) {
        QStyleOptionButton hovered = base;
        hovered.state |= QStyle::State_MouseOver;
        painter.setOpacity(t);
        style->drawControl(QStyle::CE_PushButton, &hovered, &painter, button_);
    }
    return true;
}

bool HoverFadeInstaller::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() != QEvent::Polish) return false;

    // Polish puede repetirse para el mismo widget: cada uno se instrumenta una
    // sola vez (marcado por propiedad, las clases de arriba no tienen Q_OBJECT
    // y por eso findChild<T>() no sirve).
    static const char* kInstalled = "noctisHoverFadeInstalled";
    if (watched->property(kInstalled).toBool()) return false;

    if (auto* button = qobject_cast<QPushButton*>(watched)) {
        if (qobject_cast<IconButton*>(button)) return false;
        new ButtonHoverFade(button);
    } else if (auto* scrollBar = qobject_cast<QScrollBar*>(watched)) {
        new ScrollBarHoverFade(scrollBar);
    } else if (auto* menu = qobject_cast<QMenu*>(watched)) {
        new ActionHoverFade<QMenu>(menu);
    } else if (auto* menuBar = qobject_cast<QMenuBar*>(watched)) {
        new ActionHoverFade<QMenuBar>(menuBar);
    } else if (auto* tabBar = qobject_cast<QTabBar*>(watched)) {
        new TabBarHoverFade(tabBar);
    } else {
        return false;
    }

    watched->setProperty(kInstalled, true);
    return false;
}

void installListHoverFade(QListWidget* list) {
    list->setItemDelegate(new FadeHoverDelegate(list));
}

} // namespace noctis::ui
