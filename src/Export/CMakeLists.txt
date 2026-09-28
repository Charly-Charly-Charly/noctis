add_library(noctis_export STATIC
    src/HtmlExporter.cpp
)

target_include_directories(noctis_export PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# La conversión a HTML vive en noctis_markdown (HtmlRenderer), que ya expone
# md4c-html transitivamente. Export solo se encarga de escribir el archivo.
target_link_libraries(noctis_export PUBLIC noctis_core noctis_markdown)
target_compile_features(noctis_export PUBLIC cxx_std_23)
noctis_set_warnings(noctis_export)
