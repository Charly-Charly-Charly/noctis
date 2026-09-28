#pragma once

#include "Core/Models/Folder.h"
#include "Core/Ports/INoteRepository.h"
#include "Core/Ports/ISettingsStore.h"
#include "Core/Services/ExportService.h"
#include "Core/Services/FavoritesService.h"
#include "Core/Services/FolderService.h"
#include "Core/Services/NoteService.h"
#include "Core/Services/RecentsService.h"
#include "Core/Services/SearchService.h"
#include "Core/Services/TagService.h"

namespace noctis::ui {

// Agrupa las dependencias que la UI necesita del Core: solo referencias,
// sin lógica propia. Evita que los constructores de MainWindow y los
// diálogos crezcan sin control a medida que se conectan más servicios.
struct AppContext {
    core::NoteService& noteService;
    core::FolderService& folderService;
    core::FavoritesService& favoritesService;
    core::RecentsService& recentsService;
    core::SearchService& searchService;
    core::ExportService& exportService;
    core::TagService& tagService;
    core::INoteRepository& repository;
    core::ISettingsStore& settings;
    core::Folder rootFolder;
};

} // namespace noctis::ui
