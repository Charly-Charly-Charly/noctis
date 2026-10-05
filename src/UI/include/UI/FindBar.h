#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;

namespace noctis::ui {

// Barra "buscar en la nota" (Ctrl+F) que se muestra arriba del editor. Solo
// emite señales: quién busca de verdad (editor o vista previa) lo decide
// MainWindow según el modo de vista.
class FindBar : public QWidget {
    Q_OBJECT

public:
    explicit FindBar(QWidget* parent = nullptr);

    QString query() const;

    // Muestra la barra, enfoca el campo y selecciona su texto; `initialText`
    // (no vacío) reemplaza lo escrito, p. ej. la selección actual del editor.
    void activate(const QString& initialText = QString());

    // current: posición (desde 1) de la coincidencia activa, 0 si ninguna.
    void setMatchInfo(int current, int total);

signals:
    void queryChanged(const QString& query);
    void nextRequested();
    void previousRequested();
    void closeRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QLineEdit* input_ = nullptr;
    QLabel* countLabel_ = nullptr;
};

} // namespace noctis::ui
