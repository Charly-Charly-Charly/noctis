#include "Core/Services/TagService.h"

#include <cctype>
#include <set>

namespace noctis::core {

std::vector<std::string> TagService::extractTags(const std::string& content) const {
    std::set<std::string> unique;

    for (std::size_t i = 0; i < content.size(); ++i) {
        if (content[i] != '#') continue;
        if (i > 0 && !std::isspace(static_cast<unsigned char>(content[i - 1])) &&
            content[i - 1] != '(') {
            continue; // un '#' pegado a otro carácter no cuenta como etiqueta
        }

        std::size_t start = i + 1;
        std::size_t j = start;
        while (j < content.size() &&
               (std::isalnum(static_cast<unsigned char>(content[j])) || content[j] == '-' ||
                content[j] == '_' || content[j] == '/')) {
            ++j;
        }

        if (j > start) {
            unique.insert(content.substr(start, j - start));
            i = j;
        }
    }

    return std::vector<std::string>(unique.begin(), unique.end());
}

std::string TagService::stripTags(const std::string& content) const {
    std::string result = content;
    std::size_t i = 0;

    while (i < result.size()) {
        bool validStart = result[i] == '#' &&
                           (i == 0 || std::isspace(static_cast<unsigned char>(result[i - 1])) ||
                            result[i - 1] == '(');
        if (!validStart) {
            ++i;
            continue;
        }

        std::size_t j = i + 1;
        while (j < result.size() &&
               (std::isalnum(static_cast<unsigned char>(result[j])) || result[j] == '-' ||
                result[j] == '_' || result[j] == '/')) {
            ++j;
        }

        if (j > i + 1) {
            result.erase(i, j - i); // no avanza i: sigue escaneando desde donde quedó
        } else {
            ++i;
        }
    }

    return result;
}

std::vector<TagOccurrence> TagService::collectAll(INoteRepository& repository,
                                                   const Folder& root) const {
    std::vector<TagOccurrence> occurrences;

    for (const Note& note : repository.listInFolder(root)) {
        for (const std::string& tag : extractTags(repository.readContent(note.id))) {
            occurrences.push_back({tag, note.id});
        }
    }
    for (const Folder& subfolder : repository.listSubfolders(root)) {
        std::vector<TagOccurrence> nested = collectAll(repository, subfolder);
        occurrences.insert(occurrences.end(), nested.begin(), nested.end());
    }

    return occurrences;
}

} // namespace noctis::core
