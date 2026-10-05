add_executable(noctis WIN32
    src/main.cpp
    src/MainWindow.cpp
    include/UI/MainWindow.h
    src/FolderTreeWidget.cpp
    include/UI/FolderTreeWidget.h
    src/NewNoteDialog.cpp
    include/UI/NewNoteDialog.h
    src/PreviewWidget.cpp
    include/UI/PreviewWidget.h
    src/SearchDialog.cpp
    include/UI/SearchDialog.h
    include/UI/AppContext.h
    src/Theme.cpp
    include/UI/Theme.h
    src/SyncLoginDialog.cpp
    include/UI/SyncLoginDialog.h
    src/TreeRowWidget.cpp
    include/UI/TreeRowWidget.h
    src/SidebarWidget.cpp
    include/UI/SidebarWidget.h
    src/ShortcutsDialog.cpp
    include/UI/ShortcutsDialog.h
    src/HelpDialog.cpp
    include/UI/HelpDialog.h
    src/SettingsDialog.cpp
    include/UI/SettingsDialog.h
    src/BreadcrumbWidget.cpp
    include/UI/BreadcrumbWidget.h
    src/WelcomeScreen.cpp
    include/UI/WelcomeScreen.h
    src/InsertTagDialog.cpp
    include/UI/InsertTagDialog.h
    src/FindBar.cpp
    include/UI/FindBar.h
    src/IconButton.cpp
    include/UI/IconButton.h
    src/HoverFade.cpp
    include/UI/HoverFade.h
    resources/noctis.qrc
)

if(WIN32)
    # Icono del .exe (Explorador, barra de tareas, Alt+Tab). windres (MinGW)
    # o rc.exe (MSVC) lo compilan automáticamente por ser un source .rc.
    target_sources(noctis PRIVATE resources/noctis.rc)
endif()

target_include_directories(noctis PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(noctis PRIVATE
    noctis_core
    noctis_filesystem
    noctis_database
    noctis_markdown
    noctis_search
    noctis_sync
    noctis_settings
    noctis_export
    noctis_editor
    Qt6::Widgets
)

target_compile_features(noctis PUBLIC cxx_std_23)
noctis_set_warnings(noctis)

set_target_properties(noctis PROPERTIES AUTOMOC ON AUTORCC ON)

# El corrector ortográfico busca el diccionario junto al ejecutable en
# tiempo de ejecución (ver MainWindow::setupLayout). NOCTIS_DICTIONARY_DIR lo
# fija Spelling/CMakeLists.txt al descargarlo.
if(NOCTIS_DICTIONARY_DIR AND EXISTS "${NOCTIS_DICTIONARY_DIR}/es_ES.aff")
    add_custom_command(TARGET noctis POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:noctis>/dictionaries"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${NOCTIS_DICTIONARY_DIR}/es_ES.aff" "$<TARGET_FILE_DIR:noctis>/dictionaries/es_ES.aff"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${NOCTIS_DICTIONARY_DIR}/es_ES.dic" "$<TARGET_FILE_DIR:noctis>/dictionaries/es_ES.dic"
        COMMENT "Copiando diccionario es_ES junto al ejecutable"
    )
endif()
