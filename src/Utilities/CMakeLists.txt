add_library(noctis_utilities STATIC
    src/Logger.cpp
    src/Paths.cpp
    src/Uuid.cpp
)

target_include_directories(noctis_utilities PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(noctis_utilities PUBLIC cxx_std_23)
noctis_set_warnings(noctis_utilities)
