<?php

declare(strict_types=1);

namespace Noctis;

final class SyncController
{
    private const MAX_BATCH = 500;

    public function __construct(private Database $db)
    {
    }

    /**
     * Entrega todo lo que cambió después de la revisión que trae el cliente.
     *
     * Notas y anotaciones comparten la secuencia de rev, así que un solo pull
     * deja al cliente completamente al día.
     */
    public function pull(string $userId, int $since): array
    {
        $notes = $this->db->query(
            'SELECT id, path, title, content, content_hash, rev, deleted_at
               FROM notes
              WHERE user_id = ? AND rev > ?
              ORDER BY rev
              LIMIT ' . self::MAX_BATCH,
            [$userId, $since]
        );

        $annotations = $this->db->query(
            'SELECT id, note_id, body, quote, prefix, suffix, offset_hint,
                    content_checksum, color, rev, deleted_at
               FROM annotations
              WHERE user_id = ? AND rev > ?
              ORDER BY rev
              LIMIT ' . self::MAX_BATCH,
            [$userId, $since]
        );

        $user = $this->db->queryOne('SELECT current_rev FROM users WHERE id = ?', [$userId]);
        $currentRev = (int) ($user['current_rev'] ?? 0);

        // El cliente solo puede avanzar hasta donde AMBAS listas están
        // completas. Si las notas se truncaron en rev 500 pero las anotaciones
        // llegaron hasta 900, avanzar a 900 se saltaría las notas 501-900 para
        // siempre: la marca de agua es la menor de las listas truncadas.
        $watermark = $currentRev;
        $truncated = false;

        foreach ([$notes, $annotations] as $rows) {
            if (count($rows) < self::MAX_BATCH) {
                continue; // lista completa: no limita la marca de agua
            }
            $truncated = true;
            $highest = $since;
            foreach ($rows as $row) {
                $highest = max($highest, (int) $row['rev']);
            }
            $watermark = min($watermark, $highest);
        }

        return [
            'notes'       => array_map([$this, 'presentNote'], $notes),
            'annotations' => array_map([$this, 'presentAnnotation'], $annotations),
            'rev'         => $watermark,
            'has_more'    => $truncated,
        ];
    }

    /**
     * Recibe cambios locales. Cada nota viaja con la revisión sobre la que el
     * cliente la editó (base_rev); si en el servidor ya avanzó, hay conflicto.
     *
     * Local-first significa no destruir nunca: el conflicto se devuelve para
     * que el cliente guarde una copia, y el servidor deja su versión intacta.
     */
    public function push(string $userId, array $payload): array
    {
        $notes = $payload['notes'] ?? [];
        if (count($notes) > self::MAX_BATCH) {
            throw new ApiError('Lote demasiado grande', 413);
        }

        return $this->db->transaction(function () use ($userId, $notes) {
            $applied = [];
            $conflicts = [];

            foreach ($notes as $incoming) {
                $id = $this->requireString($incoming, 'id');
                $baseRev = (int) ($incoming['base_rev'] ?? 0);

                $current = $this->db->queryOne(
                    'SELECT rev, content_hash FROM notes WHERE id = ? AND user_id = ?',
                    [$id, $userId]
                );

                if ($current !== null && (int) $current['rev'] !== $baseRev) {
                    $conflicts[] = [
                        'id'         => $id,
                        'server_rev' => (int) $current['rev'],
                        'base_rev'   => $baseRev,
                    ];
                    continue;
                }

                $rev = $this->db->nextRevision($userId);

                if (!empty($incoming['deleted'])) {
                    // Borrado lógico: un DELETE real haría que otro dispositivo
                    // volviera a subir la nota en su próximo push.
                    $this->db->execute(
                        'UPDATE notes SET deleted_at = UTC_TIMESTAMP(), rev = ?
                          WHERE id = ? AND user_id = ?',
                        [$rev, $id, $userId]
                    );
                    $applied[] = ['id' => $id, 'rev' => $rev];
                    continue;
                }

                $content = (string) ($incoming['content'] ?? '');
                $this->db->execute(
                    'INSERT INTO notes
                            (id, user_id, path, title, content, content_hash, rev)
                     VALUES (?, ?, ?, ?, ?, ?, ?)
                     ON DUPLICATE KEY UPDATE
                            path         = VALUES(path),
                            title        = VALUES(title),
                            content      = VALUES(content),
                            content_hash = VALUES(content_hash),
                            rev          = VALUES(rev),
                            deleted_at   = NULL',
                    [
                        $id,
                        $userId,
                        (string) ($incoming['path'] ?? ''),
                        (string) ($incoming['title'] ?? ''),
                        $content,
                        hash('sha256', $content),
                        $rev,
                    ]
                );

                $applied[] = ['id' => $id, 'rev' => $rev];
            }

            $user = $this->db->queryOne('SELECT current_rev FROM users WHERE id = ?', [$userId]);

            return [
                'applied'   => $applied,
                'conflicts' => $conflicts,
                'rev'       => (int) ($user['current_rev'] ?? 0),
            ];
        });
    }

    private function requireString(array $row, string $key): string
    {
        if (!isset($row[$key]) || !is_string($row[$key]) || $row[$key] === '') {
            throw new ApiError("Falta el campo '$key'", 422);
        }
        return $row[$key];
    }

    private function presentNote(array $row): array
    {
        return [
            'id'           => $row['id'],
            'path'         => $row['path'],
            'title'        => $row['title'],
            'content'      => $row['content'],
            'content_hash' => $row['content_hash'],
            'rev'          => (int) $row['rev'],
            'deleted'      => $row['deleted_at'] !== null,
        ];
    }

    private function presentAnnotation(array $row): array
    {
        return [
            'id'               => $row['id'],
            'note_id'          => $row['note_id'],
            'body'             => $row['body'],
            'quote'            => $row['quote'],
            'prefix'           => $row['prefix'],
            'suffix'           => $row['suffix'],
            'offset_hint'      => $row['offset_hint'] === null ? null : (int) $row['offset_hint'],
            'content_checksum' => $row['content_checksum'],
            'color'            => $row['color'],
            'rev'              => (int) $row['rev'],
            'deleted'          => $row['deleted_at'] !== null,
        ];
    }
}
