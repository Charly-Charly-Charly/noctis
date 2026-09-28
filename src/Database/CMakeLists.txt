include(FetchContent)

# Amalgamación oficial de SQLite: un único .c/.h, sin dependencias del
# sistema. NOTA: sin URL_HASH por ahora (scaffold inicial); fijar el hash
# SHA3-256 publicado en sqlite.org antes de considerar esto reproducible.
FetchContent_Declare(
    sqlite3
    URL https://www.sqlite.org/2024/sqlite-amalgamation-3450100.zip
)
FetchContent_MakeAvailable(sqlite3)

add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
target_compile_definitions(sqlite3 PUBLIC
    SQLITE_ENABLE_FTS5
    SQLITE_THREADSAFE=1
)
if(NOT MSVC)
    find_package(Threads REQUIRED)
    target_link_libraries(sqlite3 PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
endif()

add_library(noctis_database STATIC
    src/SqliteConnection.cpp
    src/SqliteSearchIndex.cpp
)

target_include_directories(noctis_database PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(noctis_database PUBLIC noctis_core noctis_utilities sqlite3)
target_compile_features(noctis_database PUBLIC cxx_std_23)
noctis_set_warnings(noctis_database)
