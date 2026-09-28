<?php

// Copia este archivo como config.php y ajusta los valores.
// config.php NO debe subirse a control de versiones ni quedar dentro de
// public/: contiene las credenciales de la base de datos.

return [
    'db' => [
        'host'     => getenv('NOCTIS_DB_HOST') ?: '127.0.0.1',
        'port'     => getenv('NOCTIS_DB_PORT') ?: '3306',
        'name'     => getenv('NOCTIS_DB_NAME') ?: 'noctis',
        'user'     => getenv('NOCTIS_DB_USER') ?: 'noctis',
        'password' => getenv('NOCTIS_DB_PASSWORD') ?: '',
    ],

    // Duración de la sesión de un dispositivo. Larga a propósito: el
    // escritorio sincroniza en segundo plano y no debe pedir login cada día.
    'token_lifetime_days' => 90,

    // Ponlo en false cuando el servidor esté publicado: evita filtrar detalles
    // internos en las respuestas de error.
    'debug' => (getenv('NOCTIS_DEBUG') === '1'),
];
