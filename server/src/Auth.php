<?php

declare(strict_types=1);

namespace Noctis;

final class Auth
{
    public function __construct(
        private Database $db,
        private int $tokenLifetimeDays
    ) {
    }

    public function register(string $email, string $password): array
    {
        $email = trim(strtolower($email));

        if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
            throw new ApiError('Correo inválido', 422);
        }
        // 10 caracteres como mínimo: es más efectivo que exigir símbolos.
        if (strlen($password) < 10) {
            throw new ApiError('La contraseña debe tener al menos 10 caracteres', 422);
        }

        $existing = $this->db->queryOne('SELECT id FROM users WHERE email = ?', [$email]);
        if ($existing !== null) {
            throw new ApiError('Ese correo ya está registrado', 409);
        }

        $id = Database::uuid();
        $this->db->execute(
            'INSERT INTO users (id, email, password_hash) VALUES (?, ?, ?)',
            [$id, $email, password_hash($password, PASSWORD_DEFAULT)]
        );

        return ['id' => $id, 'email' => $email];
    }

    public function login(string $email, string $password, string $deviceName): array
    {
        $email = trim(strtolower($email));
        $user = $this->db->queryOne('SELECT * FROM users WHERE email = ?', [$email]);

        // Mismo mensaje y mismo trabajo en ambos casos: distinguir "usuario no
        // existe" de "contraseña incorrecta" permitiría enumerar cuentas.
        if ($user === null) {
            password_verify($password, '$2y$10$invalidinvalidinvalidinvalidinvalidinvalidinvalidinva');
            throw new ApiError('Credenciales inválidas', 401);
        }

        if (!password_verify($password, $user['password_hash'])) {
            throw new ApiError('Credenciales inválidas', 401);
        }

        return $this->db->transaction(function () use ($user, $deviceName) {
            $deviceId = Database::uuid();
            $this->db->execute(
                'INSERT INTO devices (id, user_id, name, last_seen_at)
                 VALUES (?, ?, ?, UTC_TIMESTAMP())',
                [$deviceId, $user['id'], $deviceName !== '' ? $deviceName : 'Dispositivo']
            );

            $token = bin2hex(random_bytes(32));
            $this->db->execute(
                'INSERT INTO auth_tokens (id, user_id, device_id, token_hash, expires_at)
                 VALUES (?, ?, ?, ?, DATE_ADD(UTC_TIMESTAMP(), INTERVAL ? DAY))',
                [
                    Database::uuid(),
                    $user['id'],
                    $deviceId,
                    hash('sha256', $token),
                    $this->tokenLifetimeDays,
                ]
            );

            return [
                'token'     => $token,
                'device_id' => $deviceId,
                'user'      => ['id' => $user['id'], 'email' => $user['email']],
            ];
        });
    }

    /**
     * Resuelve el token del encabezado Authorization a una sesión activa.
     */
    public function authenticate(?string $authorizationHeader): array
    {
        if ($authorizationHeader === null
            || !preg_match('/^Bearer\s+([a-f0-9]{64})$/i', $authorizationHeader, $matches)) {
            throw new ApiError('No autenticado', 401);
        }

        $session = $this->db->queryOne(
            'SELECT t.user_id, t.device_id, u.email
               FROM auth_tokens t
               JOIN users u ON u.id = t.user_id
              WHERE t.token_hash = ? AND t.expires_at > UTC_TIMESTAMP()',
            [hash('sha256', $matches[1])]
        );

        if ($session === null) {
            throw new ApiError('Sesión expirada o inválida', 401);
        }

        if ($session['device_id'] !== null) {
            $this->db->execute(
                'UPDATE devices SET last_seen_at = UTC_TIMESTAMP() WHERE id = ?',
                [$session['device_id']]
            );
        }

        return $session;
    }

    public function logout(string $authorizationHeader): void
    {
        if (preg_match('/^Bearer\s+([a-f0-9]{64})$/i', $authorizationHeader, $matches)) {
            $this->db->execute(
                'DELETE FROM auth_tokens WHERE token_hash = ?',
                [hash('sha256', $matches[1])]
            );
        }
    }
}
