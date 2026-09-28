#include "Core/Services/LinkService.h"

namespace noctis::core {

LinkService::LinkService(INoteRepository& repository) : repository_(repository) {}

std::vector<Link> LinkService::extractLinks(const NoteId& sourceId,
                                             const std::string& content) const {
    std::vector<Link> links;

    for (std::size_t i = 0; i + 1 < content.size(); ++i) {
        if (content[i] != '[' || content[i + 1] != '[') continue;

        std::size_t start = i + 2;
        std::size_t end = content.find("]]", start);
        if (end == std::string::npos) break;

        links.push_back(Link{sourceId, content.substr(start, end - start), std::nullopt});
        i = end + 1;
    }

    return links;
}

Note LinkService::resolveOrCreate(const Folder& defaultFolder, const std::string& targetTitle) {
    if (repository_.existsInFolder(defaultFolder, targetTitle)) {
        for (const Note& note : repository_.listInFolder(defaultFolder)) {
            if (note.title == targetTitle) return note;
        }
    }

    return repository_.create(defaultFolder, targetTitle);
}

} // namespace noctis::core
