#pragma once

#include <QColor>
#include <QFont>
#include <QProxyStyle>
#include <QString>

namespace noctis::ui::theme {

// Lenguaje visual compartido por toda la app: crema/tinta o su inversión en
// modo oscuro, tipografía monoespaciada, bordes finos y tarjetas simples.
QString stylesheet(bool dark);

// Familia monoespaciada que usa toda la app (la primera instalada entre
// Cascadia Mono, JetBrains Mono, Consolas y Courier New). Es una sola a
// propósito: ver el comentario en Theme.cpp sobre el costo de memoria de las
// listas de familias.
QString monospaceFamily();

// Fuente monoespaciada para setear en QApplication.
QFont applicationFont();

// Fondo de hover del tema (más oscuro que el fondo en claro, más claro en
// oscuro). Lo leen los fundidos de HoverFade vía la propiedad de aplicación
// "noctisHoverColor", que se actualiza al cambiar de tema.
QColor hoverColor(bool dark);

// Hoja de estilos para el HTML de la vista previa (tablas, etc.): QTextDocument
// usa un subconjunto de CSS distinto al de la hoja de estilos QSS de arriba
// (no entiende selectores de widgets ni currentColor), así que no puede
// reusarla directamente.
QString previewContentStylesheet(bool dark);

// Recolorea el ícono nativo de "cerrar pestaña" según el tema activo. Ese
// pixmap lo dibuja el estilo de la plataforma, no la hoja de estilos QSS
// (que solo puede posicionarlo, no repintarlo) — por eso en modo oscuro
// terminaba prácticamente invisible sobre una pestaña sin seleccionar.
class TabCloseButtonStyle : public QProxyStyle {
public:
    explicit TabCloseButtonStyle(bool dark);

    QIcon standardIcon(StandardPixmap standardIcon, const QStyleOption* option = nullptr,
                        const QWidget* widget = nullptr) const override;

private:
    bool dark_;
};

} // namespace noctis::ui::theme
