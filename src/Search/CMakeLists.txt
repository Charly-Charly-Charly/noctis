add_library(noctis_search STATIC
    src/SearchIndexer.cpp
)

target_include_directories(noctis_search PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(noctis_search PUBLIC noctis_core)
target_compile_features(noctis_search PUBLIC cxx_std_23)
noctis_set_warnings(noctis_search)
