#pragma once

#include "Core/Ports/ISettingsStore.h"
#include "Filesystem/FilesystemNoteRepository.h"

namespace noctis::fs {

// Favoritos y recientes guardan ids de notas en Settings. Antes de los UUID
// esos ids eran rutas relativas, así que una configuración previa apunta a
// notas que ya no resuelven y se perderían en silencio.
//
// Es idempotente: un id que ya es UUID se deja intacto, así que puede correr
// en cada arranque sin llevar una bandera de "ya migrado".
class LegacyIdMigration {
public:
    LegacyIdMigration(const FilesystemNoteRepository& repository, core::ISettingsStore& settings);

    void run();

private:
    void migrateKey(const char* key);

    const FilesystemNoteRepository& repository_;
    core::ISettingsStore& settings_;
};

} // namespace noctis::fs
