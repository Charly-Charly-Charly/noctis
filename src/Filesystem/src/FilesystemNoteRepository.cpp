#include "Filesystem/FilesystemNoteRepository.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "Filesystem/Frontmatter.h"
#include "Utilities/Uuid.h"

namespace noctis::fs {

using core::Folder;
using core::Note;
using core::NoteId;

namespace {
constexpr const char* kMarkdownExtension = ".md";
constexpr const char* kIdField = "id";

std::string titleFromFilename(const std::filesystem::path& path) {
    return path.stem().string();
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("No se pudo abrir la nota: " + path.string());
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void writeFile(const std::filesystem::path& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("No se pudo escribir la nota: " + path.string());
    }
    file << content;
}
} // namespace

FilesystemNoteRepository::FilesystemNoteRepository(std::filesystem::path rootDirectory)
    : rootDirectory_(std::move(rootDirectory)) {
    reindex();
}

void FilesystemNoteRepository::reindex() {
    pathById_.clear();
    indexFolder(rootDirectory_);
}

void FilesystemNoteRepository::indexFolder(const std::filesystem::path& folder) const {
    if (!std::filesystem::exists(folder)) return;

    for (const auto& entry : std::filesystem::directory_iterator(folder)) {
        if (entry.is_directory()) {
            indexFolder(entry.path());
        } else if (entry.is_regular_file() && entry.path().extension() == kMarkdownExtension) {
            ensureIndexed(entry.path());
        }
    }
}

NoteId FilesystemNoteRepository::ensureIndexed(const std::filesystem::path& absolutePath) const {
    Document document = splitDocument(readFile(absolutePath));

    if (auto existing = frontmatterField(document.frontmatter, kIdField);
        existing && !existing->empty()) {
        pathById_[*existing] = absolutePath;
        return *existing;
    }

    // Nota nacida fuera de Noctis (o anterior a los ids): se adopta grabándole
    // un id propio, sin tocar su cuerpo.
    NoteId id = uuid::generate();
    document.frontmatter = upsertFrontmatterField(document.frontmatter, kIdField, id);
    writeFile(absolutePath, joinDocument(document));

    pathById_[id] = absolutePath;
    return id;
}

std::optional<NoteId> FilesystemNoteRepository::idForLegacyPath(
    const std::string& relativePath) const {
    std::filesystem::path absolutePath = rootDirectory_ / relativePath;

    if (!std::filesystem::exists(absolutePath)) return std::nullopt;
    if (!std::filesystem::is_regular_file(absolutePath)) return std::nullopt;
    if (absolutePath.extension() != kMarkdownExtension) return std::nullopt;

    return ensureIndexed(absolutePath);
}

std::filesystem::path FilesystemNoteRepository::pathFor(const NoteId& id) const {
    auto it = pathById_.find(id);
    if (it == pathById_.end()) {
        throw std::runtime_error("Nota desconocida: " + id);
    }
    return it->second;
}

std::optional<Note> FilesystemNoteRepository::find(const NoteId& id) const {
    auto it = pathById_.find(id);
    if (it == pathById_.end()) return std::nullopt;
    if (!std::filesystem::exists(it->second)) return std::nullopt;

    Note note;
    note.id = id;
    note.title = titleFromFilename(it->second);
    note.path = it->second;
    note.modifiedAt = std::chrono::clock_cast<std::chrono::system_clock>(
        std::filesystem::last_write_time(it->second));
    return note;
}

std::vector<Note> FilesystemNoteRepository::listInFolder(const Folder& folder) const {
    std::vector<Note> notes;

    if (!std::filesystem::exists(folder.path)) return notes;

    for (const auto& entry : std::filesystem::directory_iterator(folder.path)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != kMarkdownExtension) continue;

        if (auto note = find(ensureIndexed(entry.path()))) {
            notes.push_back(*note);
        }
    }

    return notes;
}

std::string FilesystemNoteRepository::readContent(const NoteId& id) const {
    // El editor solo ve el cuerpo: el frontmatter es metadato interno.
    return splitDocument(readFile(pathFor(id))).body;
}

void FilesystemNoteRepository::writeContent(const NoteId& id, const std::string& content) {
    std::filesystem::path path = pathFor(id);

    // Releemos para conservar el frontmatter existente (el id y cualquier otra
    // clave que el usuario haya puesto), ya que el editor no lo maneja.
    Document document = splitDocument(readFile(path));
    document.body = content;
    writeFile(path, joinDocument(document));
}

Note FilesystemNoteRepository::create(const Folder& folder, const std::string& title) {
    std::filesystem::create_directories(folder.path);

    std::filesystem::path path = folder.path / (title + kMarkdownExtension);
    if (std::filesystem::exists(path)) {
        throw std::runtime_error("Ya existe un archivo con ese nombre: " + path.string());
    }

    // El encabezado inicial es "Título, Subcarpeta, Carpeta" (de más
    // específico a más general): así queda claro de dónde viene la nota
    // incluso fuera del árbol de navegación, sin depender de rutas con "/".
    std::filesystem::path relativeFolder = std::filesystem::relative(folder.path, rootDirectory_);
    std::string heading = title;
    if (!relativeFolder.empty() && relativeFolder != ".") {
        std::vector<std::string> segments;
        for (const auto& part : relativeFolder) segments.push_back(part.string());
        for (auto it = segments.rbegin(); it != segments.rend(); ++it) {
            heading += ", " + *it;
        }
    }

    NoteId id = uuid::generate();

    Document document;
    document.frontmatter = upsertFrontmatterField("", kIdField, id);
    document.body = "# " + heading + "\n\n";
    writeFile(path, joinDocument(document));

    pathById_[id] = path;

    Note note;
    note.id = id;
    note.title = title;
    note.path = path;
    note.modifiedAt = std::chrono::system_clock::now();
    return note;
}

Note FilesystemNoteRepository::importRemote(const NoteId& id, const Folder& folder,
                                             const std::string& title,
                                             const std::string& content) {
    std::filesystem::create_directories(folder.path);

    std::filesystem::path path = folder.path / (title + kMarkdownExtension);
    if (std::filesystem::exists(path) && !pathById_.contains(id)) {
        // Un archivo local con ese nombre ya existe y es una nota distinta
        // (si fuera la misma, su id ya estaría en el índice): se desambigua
        // en vez de sobrescribir el trabajo de otra persona.
        path = folder.path / (title + " (" + id.substr(0, 8) + ")" + kMarkdownExtension);
    }

    Document document;
    document.frontmatter = upsertFrontmatterField("", kIdField, id);
    document.body = content;
    writeFile(path, joinDocument(document));

    pathById_[id] = path;

    Note note;
    note.id = id;
    note.title = title;
    note.path = path;
    note.modifiedAt = std::chrono::clock_cast<std::chrono::system_clock>(
        std::filesystem::last_write_time(path));
    return note;
}

void FilesystemNoteRepository::rename(const NoteId& id, const std::string& newTitle) {
    std::filesystem::path oldPath = pathFor(id);
    std::filesystem::path newPath = oldPath.parent_path() / (newTitle + kMarkdownExtension);

    if (std::filesystem::exists(newPath)) {
        throw std::runtime_error("Ya existe una nota con ese nombre");
    }

    std::filesystem::rename(oldPath, newPath);
    pathById_[id] = newPath; // el id no cambia: es justamente lo que lo hace estable
}

void FilesystemNoteRepository::move(const NoteId& id, const Folder& destination) {
    std::filesystem::path oldPath = pathFor(id);
    std::filesystem::path newPath = destination.path / oldPath.filename();

    std::filesystem::create_directories(destination.path);
    std::filesystem::rename(oldPath, newPath);
    pathById_[id] = newPath;
}

void FilesystemNoteRepository::remove(const NoteId& id) {
    std::filesystem::remove(pathFor(id));
    pathById_.erase(id);
}

bool FilesystemNoteRepository::existsInFolder(const Folder& folder,
                                               const std::string& title) const {
    return std::filesystem::exists(folder.path / (title + kMarkdownExtension));
}

std::vector<Folder> FilesystemNoteRepository::listSubfolders(const Folder& parent) const {
    std::vector<Folder> folders;

    if (!std::filesystem::exists(parent.path)) return folders;

    for (const auto& entry : std::filesystem::directory_iterator(parent.path)) {
        if (!entry.is_directory()) continue;
        folders.push_back(Folder{entry.path().filename().string(), entry.path()});
    }

    return folders;
}

Folder FilesystemNoteRepository::createFolder(const Folder& parent, const std::string& name) {
    std::filesystem::path path = parent.path / name;
    std::filesystem::create_directories(path);
    return Folder{name, path};
}

void FilesystemNoteRepository::renameFolder(const Folder& folder, const std::string& newName) {
    std::filesystem::path newPath = folder.path.parent_path() / newName;
    std::filesystem::rename(folder.path, newPath);
    reindex(); // cambian las rutas de todas las notas que contiene
}

void FilesystemNoteRepository::moveFolder(const Folder& folder, const Folder& destination) {
    std::filesystem::path newPath = destination.path / folder.path.filename();
    std::filesystem::rename(folder.path, newPath);
    reindex();
}

void FilesystemNoteRepository::removeFolder(const Folder& folder) {
    std::filesystem::remove_all(folder.path);
    reindex();
}

} // namespace noctis::fs
