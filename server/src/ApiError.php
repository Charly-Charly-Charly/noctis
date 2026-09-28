<?php

declare(strict_types=1);

namespace Noctis;

use RuntimeException;

// Error con un código HTTP asociado y un mensaje pensado para mostrarse al
// cliente. Cualquier otra excepción se convierte en un 500 genérico.
final class ApiError extends RuntimeException
{
    public function __construct(string $message, private int $status = 400)
    {
        parent::__construct($message);
    }

    public function status(): int
    {
        return $this->status;
    }
}
