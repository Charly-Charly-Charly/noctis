include(FetchContent)

FetchContent_Declare(
    md4c
    GIT_REPOSITORY https://github.com/mity/md4c.git
    GIT_TAG release-0.5.2
)
FetchContent_MakeAvailable(md4c)

# El CMakeLists.txt de MD4C no expone md4c.h/md4c-html.h vía
# target_include_directories para consumo directo del árbol de build (solo
# lo hace para find_package tras un install()), así que lo completamos aquí.
target_include_directories(md4c PUBLIC $<BUILD_INTERFACE:${md4c_SOURCE_DIR}/src>)
target_include_directories(md4c-html PUBLIC $<BUILD_INTERFACE:${md4c_SOURCE_DIR}/src>)

add_library(noctis_markdown STATIC
    src/MarkdownParser.cpp
    src/HtmlRenderer.cpp
)

target_include_directories(noctis_markdown PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(noctis_markdown PUBLIC noctis_core md4c md4c-html)
target_compile_features(noctis_markdown PUBLIC cxx_std_23)
noctis_set_warnings(noctis_markdown)
