<p align="center">
  <img src="image/logo.png" alt="Noctis" width="96" height="96">
</p>

<h1 align="center">Noctis</h1>

<p align="center">
  Editor de notas Markdown <strong>local-first</strong>: el disco es la fuente de verdad,
  el servidor es solo una réplica opcional.
</p>

---

## Qué es

Noctis es un editor de notas en Markdown para escritorio (Windows, Qt6), con
sincronización opcional a un servidor propio y un cliente web ligero. Cada
nota es un archivo `.md` real en tu disco — nunca queda atrapada en una base
de datos propietaria, y **nunca se sobrescribe** una edición local en
silencio: los conflictos se resuelven guardando una copia, no perdiendo
trabajo.

## Funcionalidades

- Editor y vista previa en vivo (Markdown → HTML vía [md4c](https://github.com/mity/md4c)), con scroll sincronizado en modo Split
- Árbol de carpetas/notas, favoritos y recientes
- Etiquetas (`#tag`) extraídas del propio texto — sin un store aparte
- Búsqueda instantánea (SQLite FTS5) por título, contenido, etiqueta o archivo
- Tablas, listas de tareas (`- [ ]`), citas, bloques de código — soporte GFM completo
- Pegar imágenes desde el portapapeles, guardadas junto a la nota
- Corrector ortográfico en español (Hunspell)
- Exportar a Markdown, HTML o PDF
- Modo oscuro, barra lateral colapsable, atajos de teclado tipo editor de código
- Edición tipo editor de código: multicursor (Alt+clic), cierre automático de pares, listas automáticas, zoom y búsqueda en la nota (Ctrl+F)
- Íconos vectoriales (Material Icons de Google, Apache 2.0) que siguen el tema claro/oscuro
- Aviso de nuevas versiones: consulta una vez al día el último release de GitHub (desactivable en Ajustes, sin enviar datos personales) y solo avisa; descargar lo decide el usuario
- Sincronización opcional con un servidor propio (PHP + MySQL) y un cliente web sin dependencias
- Anotaciones sobre las notas que viven solo en el servidor — nunca tocan el `.md`

## Arquitectura

Núcleo en **C++23** con arquitectura hexagonal: el dominio (`Core`) no sabe
nada de Qt, SQLite, ni del sistema de archivos — todo eso entra por puertos
(`INoteRepository`, `ISearchIndex`, `ISettingsStore`, `ISyncClient`...) que
los módulos concretos implementan.

| Módulo | Responsabilidad |
| --- | --- |
| `Core` | Modelos y casos de uso (servicios), sin dependencias externas |
| `Filesystem` | Cada nota es un `.md`, cada carpeta un directorio real |
| `Database` | Índice de búsqueda SQLite FTS5 |
| `Markdown` | Parseo/render vía md4c |
| `Sync` | Cliente HTTP hacia el servidor (WinHTTP, sin dependencias extra) |
| `Settings` | Configuración persistida en JSON |
| `Export` | Exportar a Markdown/HTML |
| `Spelling` | Corrector ortográfico (Hunspell) |
| `Editor` / `UI` | Interfaz Qt6 Widgets |

## Compilar

Requisitos: CMake 3.21+, Qt6 (Widgets), un compilador con soporte C++23
(probado con MinGW de Qt).

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<ruta-a-tu-instalación-de-Qt6>
cmake --build build
```

El corrector ortográfico y sus dependencias (Hunspell, diccionario es_ES) se
descargan solos durante la configuración. Si no hay red disponible, el
build sigue funcionando — el corrector queda deshabilitado en silencio.

### Tests

```bash
ctest --test-dir build
```

## Servidor y cliente web

Componentes opcionales para sincronizar entre dispositivos:

- [`server/`](server/README.md) — API en PHP + MySQL/MariaDB, sin dependencias
- [`web/`](web/README.md) — cliente HTML/CSS/JS puro, habla con la misma API

## Estado

Proyecto personal en desarrollo activo.
