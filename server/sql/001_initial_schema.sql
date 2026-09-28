-- Noctis — esquema inicial (MySQL 8 / MariaDB 10.4+)
--
-- El servidor NO es la fuente de verdad de las notas: el disco del escritorio
-- lo es. Aquí vive una réplica que permite leer desde el teléfono y la web, y
-- —esto sí es exclusivo del servidor— las anotaciones, que nunca se escriben
-- dentro de los .md.
--
-- Cada persona usa Noctis de forma individual: no hay notas compartidas, así
-- que todo se aísla filtrando por user_id.

SET NAMES utf8mb4;
SET time_zone = '+00:00';

-- ---------------------------------------------------------------------------
-- Usuarios
-- ---------------------------------------------------------------------------

CREATE TABLE users (
    id            CHAR(36)        NOT NULL,
    email         VARCHAR(255)    NOT NULL,
    password_hash VARCHAR(255)    NOT NULL,

    -- Contador monotónico propio de cada usuario. Cada cambio en sus notas o
    -- anotaciones consume el siguiente valor, y los clientes sincronizan
    -- pidiendo "todo lo que tenga rev mayor a la última que vi".
    current_rev   BIGINT UNSIGNED NOT NULL DEFAULT 0,

    created_at    DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (id),
    UNIQUE KEY uq_users_email (email)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COLLATE = utf8mb4_unicode_ci;

-- ---------------------------------------------------------------------------
-- Dispositivos
-- ---------------------------------------------------------------------------

CREATE TABLE devices (
    id              CHAR(36)        NOT NULL,
    user_id         CHAR(36)        NOT NULL,
    name            VARCHAR(120)    NOT NULL,

    -- Hasta dónde sincronizó este dispositivo. Permite que el escritorio, el
    -- teléfono y la web vayan cada uno a su ritmo sin pisarse.
    last_synced_rev BIGINT UNSIGNED NOT NULL DEFAULT 0,

    last_seen_at    DATETIME        NULL,
    created_at      DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (id),
    KEY idx_devices_user (user_id),
    CONSTRAINT fk_devices_user FOREIGN KEY (user_id)
        REFERENCES users (id) ON DELETE CASCADE
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COLLATE = utf8mb4_unicode_ci;

-- ---------------------------------------------------------------------------
-- Sesiones
-- ---------------------------------------------------------------------------

-- Guardamos solo el SHA-256 del token, nunca el token en claro: si alguien lee
-- la base de datos no obtiene credenciales utilizables.
CREATE TABLE auth_tokens (
    id         CHAR(36)  NOT NULL,
    user_id    CHAR(36)  NOT NULL,
    device_id  CHAR(36)  NULL,
    token_hash CHAR(64)  NOT NULL,
    expires_at DATETIME  NOT NULL,
    created_at DATETIME  NOT NULL DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (id),
    UNIQUE KEY uq_auth_tokens_hash (token_hash),
    KEY idx_auth_tokens_user (user_id),
    CONSTRAINT fk_auth_tokens_user FOREIGN KEY (user_id)
        REFERENCES users (id) ON DELETE CASCADE,
    CONSTRAINT fk_auth_tokens_device FOREIGN KEY (device_id)
        REFERENCES devices (id) ON DELETE SET NULL
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COLLATE = utf8mb4_unicode_ci;

-- ---------------------------------------------------------------------------
-- Notas (réplica del disco)
-- ---------------------------------------------------------------------------

CREATE TABLE notes (
    -- El mismo UUID que vive en el frontmatter del .md. Por eso sobrevive a
    -- renombres y movimientos, y por eso las anotaciones pueden apuntarle.
    id           CHAR(36)        NOT NULL,
    user_id      CHAR(36)        NOT NULL,

    -- La ruta es solo un metadato: cambia sin que cambie la identidad.
    path         VARCHAR(1024)   NOT NULL,
    title        VARCHAR(512)    NOT NULL,

    content      LONGTEXT        NOT NULL,
    content_hash CHAR(64)        NOT NULL,

    rev          BIGINT UNSIGNED NOT NULL,

    -- Borrado lógico: un DELETE físico no se podría propagar a los demás
    -- dispositivos, que volverían a subir la nota en el siguiente push.
    deleted_at   DATETIME        NULL,

    created_at   DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at   DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP
                                 ON UPDATE CURRENT_TIMESTAMP,

    PRIMARY KEY (id),

    -- Índice que sostiene el pull: "dame lo mío con rev > X", en orden.
    KEY idx_notes_user_rev (user_id, rev),

    CONSTRAINT fk_notes_user FOREIGN KEY (user_id)
        REFERENCES users (id) ON DELETE CASCADE
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COLLATE = utf8mb4_unicode_ci;

-- Búsqueda desde la web y el teléfono, que no tienen el índice SQLite local.
CREATE FULLTEXT INDEX ft_notes_content ON notes (title, content);

-- ---------------------------------------------------------------------------
-- Anotaciones
-- ---------------------------------------------------------------------------

-- Viven únicamente aquí: jamás se escriben dentro del archivo Markdown.
--
-- El ancla no es un número de línea (se desplazaría al editar más arriba),
-- sino la cita textual más su contexto. Al abrir la nota se resuelve en
-- cascada: offset_hint -> coincidencia exacta de quote -> búsqueda difusa con
-- prefix/suffix. Si nada calza, la anotación se muestra como huérfana en vez
-- de perderse.
CREATE TABLE annotations (
    id               CHAR(36)        NOT NULL,
    user_id          CHAR(36)        NOT NULL,
    note_id          CHAR(36)        NOT NULL,

    body             TEXT            NOT NULL,

    quote            TEXT            NOT NULL,
    prefix           VARCHAR(255)    NOT NULL DEFAULT '',
    suffix           VARCHAR(255)    NOT NULL DEFAULT '',
    offset_hint      INT UNSIGNED    NULL,

    -- Contenido de la nota cuando se creó el ancla: si no coincide, sabemos
    -- que hay que reanclar antes de confiar en offset_hint.
    content_checksum CHAR(64)        NOT NULL,

    color            VARCHAR(20)     NULL,

    -- Comparte la secuencia de rev con notes: un solo pull trae ambas cosas.
    rev              BIGINT UNSIGNED NOT NULL,

    deleted_at       DATETIME        NULL,
    created_at       DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at       DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP
                                     ON UPDATE CURRENT_TIMESTAMP,

    PRIMARY KEY (id),
    KEY idx_annotations_user_rev (user_id, rev),
    KEY idx_annotations_note (note_id, deleted_at),

    CONSTRAINT fk_annotations_user FOREIGN KEY (user_id)
        REFERENCES users (id) ON DELETE CASCADE,
    CONSTRAINT fk_annotations_note FOREIGN KEY (note_id)
        REFERENCES notes (id) ON DELETE CASCADE
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COLLATE = utf8mb4_unicode_ci;

-- ---------------------------------------------------------------------------
-- Control de versiones del esquema
-- ---------------------------------------------------------------------------

CREATE TABLE schema_migrations (
    version    VARCHAR(50) NOT NULL,
    applied_at DATETIME    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (version)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COLLATE = utf8mb4_unicode_ci;

INSERT INTO schema_migrations (version) VALUES ('001_initial_schema');
