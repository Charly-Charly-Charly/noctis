add_library(noctis_filesystem STATIC
    src/FilesystemNoteRepository.cpp
    src/FileWatcher.cpp
    src/Frontmatter.cpp
    src/LegacyIdMigration.cpp
)

target_include_directories(noctis_filesystem PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(noctis_filesystem PUBLIC noctis_core noctis_utilities)
target_compile_features(noctis_filesystem PUBLIC cxx_std_23)
noctis_set_warnings(noctis_filesystem)
