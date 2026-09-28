include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(noctis_sync STATIC
    src/ConflictDetector.cpp
    src/HttpSyncClient.cpp
)

target_include_directories(noctis_sync PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(noctis_sync PUBLIC
    noctis_core
    nlohmann_json::nlohmann_json
)
target_link_libraries(noctis_sync PRIVATE winhttp)

target_compile_features(noctis_sync PUBLIC cxx_std_23)
noctis_set_warnings(noctis_sync)
