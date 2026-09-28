#include "Filesystem/FileWatcher.h"

#include <chrono>
#include <unordered_map>

namespace noctis::fs {

FileWatcher::FileWatcher(std::filesystem::path rootDirectory)
    : rootDirectory_(std::move(rootDirectory)) {}

FileWatcher::~FileWatcher() {
    stop();
}

void FileWatcher::start(ChangeCallback onChange) {
    if (running_.exchange(true)) return;

    thread_ = std::thread([this, onChange = std::move(onChange)] {
        std::unordered_map<std::string, std::filesystem::file_time_type> known;

        while (running_.load()) {
            if (std::filesystem::exists(rootDirectory_)) {
                for (const auto& entry :
                     std::filesystem::recursive_directory_iterator(rootDirectory_)) {
                    if (!entry.is_regular_file() || entry.path().extension() != ".md") continue;

                    std::string id = entry.path().string();
                    auto writeTime = entry.last_write_time();

                    auto it = known.find(id);
                    if (it == known.end()) {
                        known[id] = writeTime;
                        onChange(id, core::ExternalChangeKind::Created);
                    } else if (it->second != writeTime) {
                        it->second = writeTime;
                        onChange(id, core::ExternalChangeKind::Modified);
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    });
}

void FileWatcher::stop() {
    if (!running_.exchange(false)) return;
    if (thread_.joinable()) thread_.join();
}

} // namespace noctis::fs
