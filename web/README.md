# Noctis — cliente web

HTML/CSS/JS puro, sin dependencias ni build step. Habla con la misma API que
usa el escritorio (ver `server/README.md`).

## Cómo funciona

- **Login** (`/auth/login`): el token se guarda en `localStorage`, nunca la
  contraseña.
- **Notas**: al iniciar, un `pull` completo trae todo. Cada edición se sube
  sola 600ms después de que dejás de escribir (debounce), igual que el
  autoguardado del escritorio.
- **Conflictos**: si el servidor cambió mientras había una edición local sin
  subir, aparece un diálogo — nunca se pisa el textarea en silencio. Podés
  quedarte con tu versión o traer la del servidor.
- **Anotaciones**: seleccioná texto en la nota y tocá el "+" del panel
  derecho. Viven solo en el servidor — nunca tocan el contenido del archivo.
- **Crear cuenta**: el mismo formulario de login tiene un enlace para
  registrarse; al crear la cuenta, entra sola sin pedir el login de nuevo.
- **Nueva nota**: formulario propio (no `prompt()`) con datalist de las
  carpetas que ya existen. El servidor no tiene tabla de carpetas — son
  implícitas por el `path` de las notas — así que una carpeta "existe" recién
  con la primera nota adentro.
- **Vista previa / Split**: un renderizador de Markdown a HTML propio y sin
  dependencias (ver `renderMarkdown` en `app.js`). Todo el texto se escapa
  antes de aplicar cualquier transformación, y los links descartan esquemas
  que no sean `http(s)`/`mailto`/rutas relativas — sin esto, una nota con
  `javascript:` en un link ejecutaría script real en el navegador (a
  diferencia del escritorio, donde `QTextBrowser` no ejecuta `<script>`).
- **Exportar a PDF**: usa el diálogo de impresión nativo del navegador
  (`window.print()` sobre un contenedor con `@media print`) en vez de una
  librería de PDF — mantiene el cliente sin dependencias.

## Despliegue

Este cliente asume que se sirve **desde el mismo origen que la API**: todas
las llamadas usan rutas absolutas como `/auth/login`, `/sync/pull`, sin
dominio ni puerto propio. `server/` en este repo ya no tiene una subcarpeta
`public/` — vive todo al mismo nivel (ver `server/README.md` para por qué) —
así que los tres archivos van directo a:

```
server/app/index.html
server/app/style.css
server/app/app.js
```

Quedará disponible en `https://tudominio.com/app/`. No hace falta tocar
`.htaccess`: el servidor sirve estos archivos directamente porque existen de
verdad en disco (el catch-all hacia `index.php` solo actúa cuando el archivo
pedido *no* existe).

## Qué falta / limitaciones conocidas

- El árbol de carpetas no tiene números de fila como el escritorio (001,
  002…) — es una lista anidada simple.
- No se puede crear una carpeta vacía (sin ninguna nota adentro): el servidor
  no tiene ese concepto, a diferencia del escritorio que crea un directorio
  real en disco.
- No hay forma de renombrar ni eliminar una nota desde la web todavía.
- No hay buscador — hay que desplegar carpetas a mano para encontrar algo.
- No hay soporte offline: sin conexión, no se puede ni leer ni editar.
- El renderizador de Markdown cubre lo más común (encabezados, listas, citas,
  código, negrita/cursiva, links) pero no es CommonMark completo — por
  ejemplo, no soporta tablas ni listas anidadas.
