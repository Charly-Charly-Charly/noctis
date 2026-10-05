#include "UI/FindBar.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>

#include "UI/IconButton.h"

namespace noctis::ui {

FindBar::FindBar(QWidget* parent) : QWidget(parent) {
    setObjectName("findBar");
    setAttribute(Qt::WA_StyledBackground, true);

    input_ = new QLineEdit(this);
    input_->setObjectName("findInput");
    input_->setPlaceholderText(tr("Buscar en la nota…"));
    input_->setClearButtonEnabled(false);
    input_->installEventFilter(this); // Enter / Shift+Enter / Esc

    countLabel_ = new QLabel(this);
    countLabel_->setObjectName("findCount");
    countLabel_->setMinimumWidth(90);
    countLabel_->setAlignment(Qt::AlignCenter);

    auto makeButton = [this](const QString& glyph, const QString& tooltip) {
        auto* button = new IconButton(glyph, this);
        button->setObjectName("findButton");
        button->setFixedSize(24, 24);
        button->setToolTip(tooltip);
        button->setFocusPolicy(Qt::NoFocus); // el foco se queda en el campo de texto
        return button;
    };
    auto* previousButton = makeButton(QString::fromUtf8("↑"), tr("Anterior (Shift+Enter)"));
    auto* nextButton = makeButton(QString::fromUtf8("↓"), tr("Siguiente (Enter)"));
    auto* closeButton = makeButton(QString::fromUtf8("✕"), tr("Cerrar (Esc)"));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(6);
    layout->addWidget(input_, 1);
    layout->addWidget(countLabel_);
    layout->addWidget(previousButton);
    layout->addWidget(nextButton);
    layout->addWidget(closeButton);

    connect(input_, &QLineEdit::textChanged, this, &FindBar::queryChanged);
    connect(previousButton, &QPushButton::clicked, this, &FindBar::previousRequested);
    connect(nextButton, &QPushButton::clicked, this, &FindBar::nextRequested);
    connect(closeButton, &QPushButton::clicked, this, &FindBar::closeRequested);

    hide();
}

QString FindBar::query() const {
    return input_->text();
}

void FindBar::activate(const QString& initialText) {
    const bool wasHidden = isHidden();
    show();
    if (!initialText.isEmpty()) input_->setText(initialText);
    input_->setFocus();
    input_->selectAll();
    // Reabrir la barra con el mismo texto de antes no dispara textChanged, y
    // sin búsqueda activa el editor no tendría nada resaltado.
    if (wasHidden && initialText.isEmpty() && !input_->text().isEmpty()) {
        emit queryChanged(input_->text());
    }
}

void FindBar::setMatchInfo(int current, int total) {
    if (input_->text().isEmpty()) {
        countLabel_->clear();
    } else if (total == 0) {
        countLabel_->setText(tr("Sin resultados"));
    } else if (current > 0) {
        countLabel_->setText(tr("%1 de %2").arg(current).arg(total));
    } else {
        countLabel_->setText(tr("%1 resultados").arg(total));
    }
}

bool FindBar::eventFilter(QObject* watched, QEvent* event) {
    if (watched == input_ && event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        switch (key->key()) {
        case Qt::Key_Escape:
            emit closeRequested();
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (key->modifiers().testFlag(Qt::ShiftModifier)) {
                emit previousRequested();
            } else {
                emit nextRequested();
            }
            return true;
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

} // namespace noctis::ui
