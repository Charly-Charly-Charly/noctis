# Noctis — servidor de sincronización y anotaciones

API en PHP + MySQL/MariaDB, sin dependencias ni Composer.

El servidor **no es la fuente de verdad**. El disco del escritorio lo es: aquí
vive una réplica que permite leer desde el teléfono y la web, más las
anotaciones, que nunca se escriben dentro de los `.md`.

---

## Qué hacer en tu servidor

### 1. Crear la base de datos y un usuario dedicado

En hosting compartido (Hostinger y similares) esto se hace desde el panel:
hPanel → Bases de datos → MySQL → Crear nueva base de datos. Ahí mismo se crea
el usuario asociado; no hace falta el `CREATE DATABASE`/`CREATE USER` a mano.

Si en cambio tenés acceso directo a MySQL como administrador:

```sql
CREATE DATABASE noctis
  CHARACTER SET utf8mb4
  COLLATE utf8mb4_unicode_ci;

CREATE USER 'noctis'@'localhost' IDENTIFIED BY 'PON_AQUI_UNA_CLAVE_LARGA';

GRANT SELECT, INSERT, UPDATE, DELETE ON noctis.* TO 'noctis'@'localhost';

FLUSH PRIVILEGES;
```

**Sin `DROP` ni `ALTER`.** La aplicación nunca necesita cambiar el esquema; las
migraciones las aplicás vos a mano con un usuario con más permisos.

Genera la contraseña con un gestor de contraseñas y guárdala ahí. No la
escribas en ningún archivo del repositorio ni la pegues en un chat: si ya la
escribiste en algún lado por accidente, rotala antes de seguir.

### 2. Aplicar el esquema

Con acceso a una terminal:

```bash
mysql -u root -p noctis < sql/001_initial_schema.sql
```

Sin terminal (hosting compartido): entrá a **phpMyAdmin** desde hPanel,
seleccioná tu base, pestaña **Importar**, subí `sql/001_initial_schema.sql`.

Verificá que haya seis tablas (`users`, `devices`, `auth_tokens`, `notes`,
`annotations`, `schema_migrations`) y que `schema_migrations` tenga la fila
`001_initial_schema`.

> Requiere MySQL 8.0+ o MariaDB 10.4+. El índice `FULLTEXT` sobre `notes` y el
> `utf8mb4` en índices de 255 caracteres necesitan esas versiones o superiores.

### 3. Subir los archivos

**Estructura de este repositorio** (todo al mismo nivel, sin subcarpeta
`public/`):

```
server/
├── config.php          <- lo creás en el paso 4
├── sql/
├── src/
├── app/                 <- cliente web (ver web/README.md)
├── index.php
└── .htaccess
```

Esto es a propósito: muchos hostings compartidos (Hostinger incluido, según
el plan) no dejan elegir un Document Root distinto de la carpeta del
dominio/subdominio, así que no siempre se puede sacar `src/` y `config.php`
fuera de la zona pública. En vez de depender de eso, `.htaccess` les niega el
acceso directo por HTTP con reglas `RewriteRule ^(src|sql)/ - [F,L]` y
`<FilesMatch>` sobre `config.php`. PHP los sigue leyendo perfectamente
(`require()` no pasa por Apache), pero un navegador que intente pedirlos
directo recibe 403.

Subí la carpeta `server/` **completa**, manteniendo esta estructura plana, y
apuntá el Document Root del dominio/subdominio directamente a esa carpeta
(no hace falta — ni conviene forzar — una subcarpeta `public/` aparte).

> Si tu hosting **sí** te deja elegir un Document Root distinto de la carpeta
> del dominio, podés en cambio mover `index.php`, `.htaccess` y `app/` a una
> subcarpeta `public/` y apuntar el Document Root ahí, dejando `src/`,
> `sql/` y `config.php` un nivel afuera — es más simple todavía porque ni
> hace falta el bloqueo por `.htaccess`. En ese caso cambiá también las rutas
> de `require` en `index.php` de `__DIR__ . '/src/...'` a
> `__DIR__ . '/../src/...'` (y lo mismo para `config.php`).

### 4. Crear `config.php`

```bash
cp config.example.php config.php
```

Editalo con los datos reales de la base (los mismos que ves en hPanel →
Bases de datos → MySQL). Si tu hosting permite variables de entorno, usalas
en vez de escribir la contraseña en el archivo — el ejemplo ya las lee con
`getenv()`.

Con acceso a shell, asegurá los permisos:

```bash
chmod 640 config.php
```

### 5. Habilitar HTTPS — obligatorio

Los tokens de sesión viajan en el encabezado `Authorization`. Sobre HTTP plano
cualquiera en la red puede leerlos y suplantar la sesión. La mayoría de los
hostings compartidos (Hostinger incluido) traen Let's Encrypt con un clic
desde el panel (hPanel → SSL). En un servidor propio:

```bash
sudo certbot --nginx -d noctis.tudominio.com
```

No pongas esto en producción sin TLS.

### 6. Probar que responde

```bash
curl https://noctis.tudominio.com/health
```

Debe devolver `{"status":"ok"}`.

Si en cambio ves...

- **La página de error genérica del hosting** (no un JSON): el
  Document Root no está apuntando a esta carpeta, o `index.php` todavía no
  se subió/está en el lugar equivocado.
- **500 con el cuerpo vacío**: casi siempre es un `require()` que no
  encuentra un archivo — revisá que `src/` esté al mismo nivel que
  `index.php` (paso 3), y que los nombres de archivo tengan exactamente las
  mayúsculas de este repo (Linux distingue mayúsculas de minúsculas, Windows
  no, así que un FTP mal configurado a veces las cambia).
- **500 con un mensaje JSON tipo "Falta config.php"**: el paso 4 quedó
  incompleto.
- **El código PHP como texto plano** en vez de ejecutarse: problema de
  configuración de PHP del lado del hosting, no de este proyecto — contactá
  soporte de tu hosting.

### 7. Crear tu cuenta

```bash
curl -X POST https://noctis.tudominio.com/auth/register -H "Content-Type: application/json" -d "{\"email\":\"tu@correo.com\",\"password\":\"una-clave-de-al-menos-10-caracteres\"}"
```

O desde la consola del navegador (F12 → Console):

```js
fetch('https://noctis.tudominio.com/auth/register', {
  method: 'POST',
  headers: {'Content-Type': 'application/json'},
  body: JSON.stringify({email: 'tu@correo.com', password: 'una-clave-de-al-menos-10-caracteres'})
}).then(r => r.json()).then(console.log)
```

Después iniciás sesión desde el escritorio (menú Cuenta) o desde
`https://noctis.tudominio.com/app/` (el cliente web).

---

## Endpoints

| Método | Ruta | Qué hace |
|--------|------|----------|
| `POST` | `/auth/register` | Crea una cuenta |
| `POST` | `/auth/login` | Devuelve token y `device_id` |
| `POST` | `/auth/logout` | Invalida el token actual |
| `GET`  | `/health` | Comprobación de vida |
| `GET`  | `/sync/pull?since=N` | Cambios con `rev > N` |
| `POST` | `/sync/push` | Sube cambios locales |
| `GET`  | `/notes/{id}/annotations` | Anotaciones de una nota |
| `POST` | `/annotations` | Crea una anotación |
| `PATCH`| `/annotations/{id}` | Edita el texto |
| `DELETE`| `/annotations/{id}` | Borrado lógico |

Todo excepto `/auth/*` y `/health` exige `Authorization: Bearer <token>`.

## Cómo funciona la sincronización

Cada usuario tiene un contador `current_rev`. Cada cambio consume el siguiente
valor, y notas y anotaciones comparten esa secuencia — por eso un solo `pull`
deja al cliente completamente al día.

El `push` envía cada nota con la revisión sobre la que se editó (`base_rev`).
Si en el servidor ya avanzó, es un conflicto: el servidor **no sobrescribe**,
devuelve el conflicto y el cliente preserva la edición local en vez de
perderla (el escritorio la guarda como copia `(conflicto)`; el cliente web
muestra un diálogo para elegir qué hacer). Local-first significa no destruir
nunca trabajo del usuario.

Los borrados son lógicos (`deleted_at`). Un `DELETE` físico no se podría
propagar: el siguiente dispositivo en sincronizar volvería a subir la nota.

## Anotaciones

No tocan el archivo. El ancla no es un número de línea —se desplazaría al
editar más arriba— sino la cita textual (`quote`) con su contexto (`prefix`,
`suffix`) y un `offset_hint`. Al abrir la nota se resuelve en cascada:

1. `offset_hint`, si `content_checksum` coincide con el contenido actual
2. coincidencia exacta de `quote`
3. búsqueda difusa usando `prefix`/`suffix`

Si nada calza, la anotación se marca huérfana y se muestra aparte en vez de
perderse.

## Estado

Esquema, API, cliente de escritorio (`Sync/HttpSyncClient` +
`Core/Services/SyncService`) y cliente web (`web/`) están probados de punta a
punta contra un servidor MySQL/MariaDB real: login, pull, push, conflictos y
anotaciones, verificados con peticiones reales, no solo revisión de código.
