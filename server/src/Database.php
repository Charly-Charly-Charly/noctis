<?php

declare(strict_types=1);

namespace Noctis;

use PDO;
use PDOException;
use RuntimeException;

final class Database
{
    private PDO $pdo;

    public function __construct(array $config)
    {
        $dsn = sprintf(
            'mysql:host=%s;port=%s;dbname=%s;charset=utf8mb4',
            $config['host'],
            $config['port'],
            $config['name']
        );

        try {
            $this->pdo = new PDO($dsn, $config['user'], $config['password'], [
                PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
                PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
                // Sentencias preparadas reales, no emuladas: la emulación
                // interpola en el cliente y pierde parte de la protección.
                PDO::ATTR_EMULATE_PREPARES   => false,
            ]);
        } catch (PDOException $e) {
            throw new RuntimeException('No se pudo conectar a la base de datos', 0, $e);
        }
    }

    public function pdo(): PDO
    {
        return $this->pdo;
    }

    public function query(string $sql, array $params = []): array
    {
        $statement = $this->pdo->prepare($sql);
        $statement->execute($params);
        return $statement->fetchAll();
    }

    public function queryOne(string $sql, array $params = []): ?array
    {
        $rows = $this->query($sql, $params);
        return $rows[0] ?? null;
    }

    public function execute(string $sql, array $params = []): int
    {
        $statement = $this->pdo->prepare($sql);
        $statement->execute($params);
        return $statement->rowCount();
    }

    public function transaction(callable $work): mixed
    {
        $this->pdo->beginTransaction();
        try {
            $result = $work();
            $this->pdo->commit();
            return $result;
        } catch (\Throwable $e) {
            $this->pdo->rollBack();
            throw $e;
        }
    }

    /**
     * Reserva el siguiente número de revisión del usuario.
     *
     * Se hace con un UPDATE atómico en vez de "leer y luego escribir" para que
     * dos dispositivos sincronizando a la vez no reciban el mismo rev.
     * Debe llamarse dentro de una transacción.
     */
    public function nextRevision(string $userId): int
    {
        $this->execute(
            'UPDATE users SET current_rev = current_rev + 1 WHERE id = ?',
            [$userId]
        );

        $row = $this->queryOne('SELECT current_rev FROM users WHERE id = ?', [$userId]);
        if ($row === null) {
            throw new RuntimeException('Usuario inexistente');
        }

        return (int) $row['current_rev'];
    }

    public static function uuid(): string
    {
        $bytes = random_bytes(16);
        $bytes[6] = chr((ord($bytes[6]) & 0x0F) | 0x40); // versión 4
        $bytes[8] = chr((ord($bytes[8]) & 0x3F) | 0x80); // variante RFC 4122

        return vsprintf('%s%s-%s-%s-%s-%s%s%s', str_split(bin2hex($bytes), 4));
    }
}
