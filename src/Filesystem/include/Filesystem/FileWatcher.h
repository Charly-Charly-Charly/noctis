#pragma once

#include <atomic>
#include <filesystem>
#include <thread>

#include "Core/Ports/ISyncWatcher.h"

namespace noctis::fs {

// Implementación inicial por polling. Cumple el contrato de ISyncWatcher
// pero no el objetivo de "CPU en reposo ~0"; sustituir por notificaciones
// nativas del SO (ReadDirectoryChangesW / inotify / FSEvents) antes de v1.
class FileWatcher : public core::ISyncWatcher {
public:
    explicit FileWatcher(std::filesystem::path rootDirectory);
    ~FileWatcher() override;

    void start(ChangeCallback onChange) override;
    void stop() override;

private:
    std::filesystem::path rootDirectory_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace noctis::fs
