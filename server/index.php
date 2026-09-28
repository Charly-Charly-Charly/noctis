<?php

declare(strict_types=1);

// Vive al mismo nivel que src/ y config.php a propósito: muchos hostings
// compartidos no dejan elegir un Document Root distinto de la carpeta del
// dominio, así que no hay forma de sacar src/config.php de ahí. En vez de
// eso, el .htaccess de al lado les niega el acceso directo por HTTP —
// PHP sigue pudiendo leerlos porque require() no pasa por Apache.

require __DIR__ . '/src/ApiError.php';
require __DIR__ . '/src/Database.php';
require __DIR__ . '/src/Auth.php';
require __DIR__ . '/src/SyncController.php';
require __DIR__ . '/src/AnnotationController.php';

use Noctis\AnnotationController;
use Noctis\ApiError;
use Noctis\Auth;
use Noctis\Database;
use Noctis\SyncController;

header('Content-Type: application/json; charset=utf-8');
header('X-Content-Type-Options: nosniff');

$configPath = __DIR__ . '/config.php';
if (!file_exists($configPath)) {
    http_response_code(500);
    echo json_encode(['error' => 'Falta config.php: copia config.example.php y complétalo']);
    exit;
}
$config = require $configPath;

function respond(array $payload, int $status = 200): never
{
    http_response_code($status);
    echo json_encode($payload, JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    exit;
}

function jsonBody(): array
{
    $raw = file_get_contents('php://input');
    if ($raw === false || $raw === '') {
        return [];
    }
    $decoded = json_decode($raw, true);
    if (!is_array($decoded)) {
        throw new ApiError('Cuerpo JSON inválido', 400);
    }
    return $decoded;
}

function authorizationHeader(): ?string
{
    // Apache no siempre expone Authorization en $_SERVER sin el RewriteRule
    // del .htaccess; getallheaders() cubre el resto de los casos.
    if (isset($_SERVER['HTTP_AUTHORIZATION'])) {
        return $_SERVER['HTTP_AUTHORIZATION'];
    }
    if (isset($_SERVER['REDIRECT_HTTP_AUTHORIZATION'])) {
        return $_SERVER['REDIRECT_HTTP_AUTHORIZATION'];
    }
    if (function_exists('getallheaders')) {
        foreach (getallheaders() as $name => $value) {
            if (strcasecmp($name, 'Authorization') === 0) {
                return $value;
            }
        }
    }
    return null;
}

$method = $_SERVER['REQUEST_METHOD'] ?? 'GET';
$path = parse_url($_SERVER['REQUEST_URI'] ?? '/', PHP_URL_PATH) ?: '/';

// Tolera que la API viva en un subdirectorio (…/api/v1/sync/pull).
if (preg_match('#/(?:api)(?:/v1)?(/.*)$#', $path, $matches)) {
    $path = $matches[1];
}
$path = rtrim($path, '/') ?: '/';

try {
    $db = new Database($config['db']);
    $auth = new Auth($db, (int) $config['token_lifetime_days']);

    // --- Rutas públicas ----------------------------------------------------

    if ($method === 'POST' && $path === '/auth/register') {
        $body = jsonBody();
        respond($auth->register(
            (string) ($body['email'] ?? ''),
            (string) ($body['password'] ?? '')
        ), 201);
    }

    if ($method === 'POST' && $path === '/auth/login') {
        $body = jsonBody();
        respond($auth->login(
            (string) ($body['email'] ?? ''),
            (string) ($body['password'] ?? ''),
            (string) ($body['device_name'] ?? 'Dispositivo')
        ));
    }

    if ($method === 'GET' && $path === '/health') {
        respond(['status' => 'ok']);
    }

    // --- A partir de aquí se exige sesión ----------------------------------

    $session = $auth->authenticate(authorizationHeader());
    $userId = $session['user_id'];

    if ($method === 'POST' && $path === '/auth/logout') {
        $auth->logout((string) authorizationHeader());
        respond(['status' => 'ok']);
    }

    $sync = new SyncController($db);

    if ($method === 'GET' && $path === '/sync/pull') {
        respond($sync->pull($userId, (int) ($_GET['since'] ?? 0)));
    }

    if ($method === 'POST' && $path === '/sync/push') {
        respond($sync->push($userId, jsonBody()));
    }

    $annotations = new AnnotationController($db);

    if ($method === 'GET' && preg_match('#^/notes/([\w-]{36})/annotations$#', $path, $m)) {
        respond($annotations->listForNote($userId, $m[1]));
    }

    if ($method === 'POST' && $path === '/annotations') {
        respond($annotations->create($userId, jsonBody()), 201);
    }

    if ($method === 'PATCH' && preg_match('#^/annotations/([\w-]{36})$#', $path, $m)) {
        respond($annotations->update($userId, $m[1], jsonBody()));
    }

    if ($method === 'DELETE' && preg_match('#^/annotations/([\w-]{36})$#', $path, $m)) {
        respond($annotations->delete($userId, $m[1]));
    }

    respond(['error' => 'Ruta no encontrada'], 404);
} catch (ApiError $e) {
    respond(['error' => $e->getMessage()], $e->status());
} catch (Throwable $e) {
    // Nunca devolvemos el detalle interno salvo en depuración: filtraría
    // rutas del servidor y estructura de la base de datos.
    error_log('[noctis] ' . $e->getMessage());
    respond([
        'error' => empty($config['debug']) ? 'Error interno' : $e->getMessage(),
    ], 500);
}
