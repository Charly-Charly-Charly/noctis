#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "Core/Ports/INoteRepository.h"

namespace noctis::fs {

// Implementación real de INoteRepository: cada Note es literalmente un
// archivo .md en disco, cada Folder es literalmente un directorio del SO.
// Esta clase es la única frontera que toca std::filesystem para notas.
//
// La identidad de una nota es un UUID guardado en su frontmatter, no su ruta:
// así sobrevive a renombres y movimientos (incluidos los hechos por otra
// herramienta de sync) y las anotaciones remotas pueden apuntar a ella sin
// romperse. La ruta pasa a ser un simple metadato.
class FilesystemNoteRepository : public core::INoteRepository {
public:
    explicit FilesystemNoteRepository(std::filesystem::path rootDirectory);

    std::optional<core::Note> find(const core::NoteId& id) const override;
    std::vector<core::Note> listInFolder(const core::Folder& folder) const override;

    std::string readContent(const core::NoteId& id) const override;
    void writeContent(const core::NoteId& id, const std::string& content) override;

    core::Note create(const core::Folder& folder, const std::string& title) override;
    core::Note importRemote(const core::NoteId& id, const core::Folder& folder,
                             const std::string& title, const std::string& content) override;
    void rename(const core::NoteId& id, const std::string& newTitle) override;
    void move(const core::NoteId& id, const core::Folder& destination) override;
    void remove(const core::NoteId& id) override;

    bool existsInFolder(const core::Folder& folder, const std::string& title) const override;

    std::vector<core::Folder> listSubfolders(const core::Folder& parent) const override;
    core::Folder createFolder(const core::Folder& parent, const std::string& name) override;
    void renameFolder(const core::Folder& folder, const std::string& newName) override;
    void moveFolder(const core::Folder& folder, const core::Folder& destination) override;
    void removeFolder(const core::Folder& folder) override;

    // Reconstruye el índice id→ruta recorriendo el árbol. Necesario tras
    // cambios hechos fuera de Noctis (Syncthing, el explorador de archivos).
    void reindex();

    // Traduce un id heredado (la ruta relativa que se usaba como identidad
    // antes de los UUID) al id actual de esa nota. Devuelve nullopt si la ruta
    // ya no corresponde a ninguna nota.
    std::optional<core::NoteId> idForLegacyPath(const std::string& relativePath) const;

private:
    // Devuelve el id del archivo, asignándole uno nuevo y escribiéndolo en su
    // frontmatter si aún no lo tiene. Es lo que adopta notas creadas fuera de
    // Noctis sin pedirle nada al usuario.
    core::NoteId ensureIndexed(const std::filesystem::path& absolutePath) const;
    std::filesystem::path pathFor(const core::NoteId& id) const;
    void indexFolder(const std::filesystem::path& folder) const;

    std::filesystem::path rootDirectory_;

    // Índice en memoria; mutable porque find()/listInFolder() son const pero
    // deben poder adoptar archivos que aparecieron en disco desde el último
    // recorrido.
    mutable std::unordered_map<core::NoteId, std::filesystem::path> pathById_;
};

} // namespace noctis::fs
