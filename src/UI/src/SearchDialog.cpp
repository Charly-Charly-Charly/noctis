#include "UI/SearchDialog.h"

#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace noctis::ui {

namespace {
constexpr int kIdRole = Qt::UserRole;
} // namespace

SearchDialog::SearchDialog(core::SearchService& searchService, QWidget* parent,
                            const QString& initialQuery)
    : QDialog(parent), searchService_(searchService) {
    setWindowTitle(tr("Buscar"));
    resize(480, 360);

    queryEdit_ = new QLineEdit(this);
    queryEdit_->setPlaceholderText(tr("Título, contenido, etiqueta o archivo…"));
    resultsList_ = new QListWidget(this);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(queryEdit_);
    layout->addWidget(resultsList_);

    connect(queryEdit_, &QLineEdit::textChanged, this, &SearchDialog::handleTextChanged);
    connect(queryEdit_, &QLineEdit::returnPressed, this, &SearchDialog::openSelected);
    connect(resultsList_, &QListWidget::itemActivated, this, &SearchDialog::handleItemActivated);

    queryEdit_->installEventFilter(this);

    if (!initialQuery.isEmpty()) {
        queryEdit_->setText(initialQuery); // dispara handleTextChanged vía el signal
    }
    queryEdit_->setFocus();
}

bool SearchDialog::eventFilter(QObject* watched, QEvent* event) {
    if (watched == queryEdit_ && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Down || keyEvent->key() == Qt::Key_Up) {
            int row = resultsList_->currentRow();
            int newRow = keyEvent->key() == Qt::Key_Down ? row + 1 : row - 1;
            newRow = std::clamp(newRow, 0, resultsList_->count() - 1);
            resultsList_->setCurrentRow(newRow);
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void SearchDialog::handleTextChanged(const QString& text) {
    resultsList_->clear();
    if (text.trimmed().isEmpty()) return;

    for (const core::SearchResult& result : searchService_.search(text.toStdString())) {
        auto* item = new QListWidgetItem(QString::fromStdString(result.title), resultsList_);
        item->setData(kIdRole, QString::fromStdString(result.noteId));
        if (!result.snippet.empty()) {
            item->setToolTip(QString::fromStdString(result.snippet));
        }
    }

    if (resultsList_->count() > 0) {
        resultsList_->setCurrentRow(0);
    }
}

void SearchDialog::openSelected() {
    if (auto* item = resultsList_->currentItem()) {
        handleItemActivated(item);
    }
}

void SearchDialog::handleItemActivated(QListWidgetItem* item) {
    emit noteSelected(item->data(kIdRole).toString().toStdString());
    accept();
}

} // namespace noctis::ui
