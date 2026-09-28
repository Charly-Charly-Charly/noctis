add_library(noctis_core STATIC
    src/NoteService.cpp
    src/FolderService.cpp
    src/TagService.cpp
    src/LinkService.cpp
    src/SearchService.cpp
    src/FavoritesService.cpp
    src/RecentsService.cpp
    src/ExportService.cpp
    src/SyncService.cpp
)

target_include_directories(noctis_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(noctis_core PUBLIC noctis_utilities)
target_compile_features(noctis_core PUBLIC cxx_std_23)
noctis_set_warnings(noctis_core)
