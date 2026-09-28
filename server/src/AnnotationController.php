<?php

declare(strict_types=1);

namespace Noctis;

// Las anotaciones son lo único que existe solo en el servidor: nunca se
// escriben dentro del .md. Por eso pueden crearse desde el teléfono o la web
// sin tocar el archivo que vive en el disco del escritorio.
final class AnnotationController
{
    private const MAX_CONTEXT = 255;

    public function __construct(private Database $db)
    {
    }

    public function listForNote(string $userId, string $noteId): array
    {
        $rows = $this->db->query(
            'SELECT id, note_id, body, quote, prefix, suffix, offset_hint,
                    content_checksum, color, rev
               FROM annotations
              WHERE user_id = ? AND note_id = ? AND deleted_at IS NULL
              ORDER BY offset_hint IS NULL, offset_hint, created_at',
            [$userId, $noteId]
        );

        return ['annotations' => $rows];
    }

    public function create(string $userId, array $input): array
    {
        $noteId = $this->requireString($input, 'note_id');
        $body = $this->requireString($input, 'body');
        $quote = $this->requireString($input, 'quote');

        $note = $this->db->queryOne(
            'SELECT content FROM notes WHERE id = ? AND user_id = ? AND deleted_at IS NULL',
            [$noteId, $userId]
        );
        if ($note === null) {
            throw new ApiError('La nota no existe', 404);
        }

        return $this->db->transaction(function () use ($userId, $noteId, $input, $body, $quote, $note) {
            $id = Database::uuid();
            $rev = $this->db->nextRevision($userId);

            $this->db->execute(
                'INSERT INTO annotations
                        (id, user_id, note_id, body, quote, prefix, suffix,
                         offset_hint, content_checksum, color, rev)
                 VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)',
                [
                    $id,
                    $userId,
                    $noteId,
                    $body,
                    $quote,
                    mb_substr((string) ($input['prefix'] ?? ''), 0, self::MAX_CONTEXT),
                    mb_substr((string) ($input['suffix'] ?? ''), 0, self::MAX_CONTEXT),
                    isset($input['offset_hint']) ? (int) $input['offset_hint'] : null,
                    // Si el cliente no lo manda, lo calculamos del contenido
                    // actual: así siempre se sabe contra qué versión se ancló.
                    (string) ($input['content_checksum'] ?? hash('sha256', $note['content'])),
                    $input['color'] ?? null,
                    $rev,
                ]
            );

            return ['id' => $id, 'rev' => $rev];
        });
    }

    public function update(string $userId, string $id, array $input): array
    {
        $body = $this->requireString($input, 'body');

        return $this->db->transaction(function () use ($userId, $id, $body) {
            $rev = $this->db->nextRevision($userId);

            $changed = $this->db->execute(
                'UPDATE annotations SET body = ?, rev = ?
                  WHERE id = ? AND user_id = ? AND deleted_at IS NULL',
                [$body, $rev, $id, $userId]
            );

            if ($changed === 0) {
                throw new ApiError('La anotación no existe', 404);
            }

            return ['id' => $id, 'rev' => $rev];
        });
    }

    public function delete(string $userId, string $id): array
    {
        return $this->db->transaction(function () use ($userId, $id) {
            $rev = $this->db->nextRevision($userId);

            $changed = $this->db->execute(
                'UPDATE annotations SET deleted_at = UTC_TIMESTAMP(), rev = ?
                  WHERE id = ? AND user_id = ? AND deleted_at IS NULL',
                [$rev, $id, $userId]
            );

            if ($changed === 0) {
                throw new ApiError('La anotación no existe', 404);
            }

            return ['id' => $id, 'rev' => $rev];
        });
    }

    private function requireString(array $input, string $key): string
    {
        if (!isset($input[$key]) || !is_string($input[$key]) || trim($input[$key]) === '') {
            throw new ApiError("Falta el campo '$key'", 422);
        }
        return $input[$key];
    }
}
