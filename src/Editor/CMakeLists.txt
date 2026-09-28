add_library(noctis_editor STATIC
    src/EditorWidget.cpp
    include/Editor/EditorWidget.h
    src/SpellCheckHighlighter.cpp
    include/Editor/SpellCheckHighlighter.h
)

target_include_directories(noctis_editor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(noctis_editor PUBLIC noctis_core noctis_spelling Qt6::Widgets)
target_compile_features(noctis_editor PUBLIC cxx_std_23)
noctis_set_warnings(noctis_editor)

set_target_properties(noctis_editor PROPERTIES AUTOMOC ON)
