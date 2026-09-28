#pragma once

#include <QDialog>

#include <string>
#include <vector>

class QComboBox;

namespace noctis::ui {

// Un combo editable con las etiquetas ya usadas en el vault (autocompletar)
// más la posibilidad de escribir una nueva. No crea nada por sí solo: solo
// devuelve el texto para que MainWindow lo inserte como "#tag " en el
// cursor — las etiquetas siguen siendo texto Markdown plano (ver
// TagService), no hay un store aparte.
class InsertTagDialog : public QDialog {
    Q_OBJECT

public:
    InsertTagDialog(const std::vector<std::string>& existingTags, QWidget* parent = nullptr);

    // Ya sin "#" ni espacios (ver TagService::extractTags: un espacio
    // cortaría la etiqueta ahí mismo).
    QString tag() const;

private:
    QComboBox* tagCombo_;
};

} // namespace noctis::ui
