include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(noctis_settings STATIC
    src/JsonSettingsStore.cpp
)

target_include_directories(noctis_settings PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(noctis_settings PUBLIC
    noctis_core
    noctis_utilities
    nlohmann_json::nlohmann_json
)
target_compile_features(noctis_settings PUBLIC cxx_std_23)
noctis_set_warnings(noctis_settings)
