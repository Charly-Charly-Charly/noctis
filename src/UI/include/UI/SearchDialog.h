#pragma once

#include <QDialog>

#include "Core/Services/SearchService.h"

class QLineEdit;
class QListWidget;
class QListWidgetItem;

namespace noctis::ui {

// Búsqueda instantánea sobre título, contenido, etiquetas y nombre de
// archivo, delegada en SearchService (SQLite FTS5 por debajo).
class SearchDialog : public QDialog {
    Q_OBJECT

public:
    SearchDialog(core::SearchService& searchService, QWidget* parent = nullptr,
                 const QString& initialQuery = QString());

signals:
    void noteSelected(const core::NoteId& id);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void handleTextChanged(const QString& text);
    void openSelected();
    void handleItemActivated(QListWidgetItem* item);

    core::SearchService& searchService_;
    QLineEdit* queryEdit_;
    QListWidget* resultsList_;
};

} // namespace noctis::ui
