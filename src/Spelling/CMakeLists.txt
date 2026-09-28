include(FetchContent)

# Hunspell no tiene CMakeLists.txt propio (usa autotools) — FetchContent solo
# descarga el código y CMake no intenta configurarlo como subproyecto. Se
# compila acá directamente el motor (src/hunspell/*.cxx), igual que ya se
# hace con la amalgamación de sqlite3 en Database/CMakeLists.txt. NOTA: sin
# verificación de hash por ahora, mismo scaffold inicial que sqlite3 — fijar
# antes de considerar esto reproducible.
FetchContent_Declare(
    hunspell
    GIT_REPOSITORY https://github.com/hunspell/hunspell.git
    GIT_TAG v1.7.3
)
FetchContent_MakeAvailable(hunspell)

add_library(hunspell STATIC
    ${hunspell_SOURCE_DIR}/src/hunspell/affentry.cxx
    ${hunspell_SOURCE_DIR}/src/hunspell/affixmgr.cxx
    ${hunspell_SOURCE_DIR}/src/hunspell/csutil.cxx
    ${hunspell_SOURCE_DIR}/src/hunspell/filemgr.cxx
    ${hunspell_SOURCE_DIR}/src/hunspell/hashmgr.cxx
    ${hunspell_SOURCE_DIR}/src/hunspell/hunspell.cxx
    ${hunspell_SOURCE_DIR}/src/hunspell/hunzip.cxx
    ${hunspell_SOURCE_DIR}/src/hunspell/phonet.cxx
    ${hunspell_SOURCE_DIR}/src/hunspell/replist.cxx
    ${hunspell_SOURCE_DIR}/src/hunspell/suggestmgr.cxx
)
target_include_directories(hunspell PUBLIC ${hunspell_SOURCE_DIR}/src/hunspell)
# HUNSPELL_STATIC anula los declspec dllexport/dllimport de hunvisapi.h: sin
# esto, MinGW no encuentra los símbolos al linkear estático.
target_compile_definitions(hunspell PUBLIC HUNSPELL_STATIC)
target_compile_features(hunspell PRIVATE cxx_std_17)

# Diccionario es_ES (LibreOffice, triple licencia GPLv3/LGPLv3/MPL1.1),
# servido como archivos .aff/.dic sueltos por wooorm/dictionaries en vez de
# tener que clonar el monorepo completo de LibreOffice/dictionaries. Se
# descarga una sola vez (no en cada reconfigure) y se guarda como variable de
# caché para que UI/CMakeLists.txt pueda copiarlo junto al ejecutable.
set(NOCTIS_DICTIONARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/dictionary" CACHE INTERNAL "")

if(NOT EXISTS "${NOCTIS_DICTIONARY_DIR}/es_ES.aff")
    file(MAKE_DIRECTORY "${NOCTIS_DICTIONARY_DIR}")
    message(STATUS "Descargando diccionario es_ES para el corrector ortográfico...")

    file(DOWNLOAD
        "https://raw.githubusercontent.com/wooorm/dictionaries/main/dictionaries/es/index.aff"
        "${NOCTIS_DICTIONARY_DIR}/es_ES.aff" STATUS affStatus)
    file(DOWNLOAD
        "https://raw.githubusercontent.com/wooorm/dictionaries/main/dictionaries/es/index.dic"
        "${NOCTIS_DICTIONARY_DIR}/es_ES.dic" STATUS dicStatus)
    file(DOWNLOAD
        "https://raw.githubusercontent.com/wooorm/dictionaries/main/dictionaries/es/license"
        "${NOCTIS_DICTIONARY_DIR}/LICENSE-es_ES.txt")

    list(GET affStatus 0 affCode)
    list(GET dicStatus 0 dicCode)
    if(NOT affCode EQUAL 0 OR NOT dicCode EQUAL 0)
        message(WARNING "No se pudo descargar el diccionario es_ES: el corrector "
                         "ortográfico quedará deshabilitado hasta reconfigurar con red "
                         "disponible (SpellChecker::isAvailable() degrada en silencio).")
        file(REMOVE "${NOCTIS_DICTIONARY_DIR}/es_ES.aff" "${NOCTIS_DICTIONARY_DIR}/es_ES.dic")
    endif()
endif()

add_library(noctis_spelling STATIC
    src/SpellChecker.cpp
)

target_include_directories(noctis_spelling PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(noctis_spelling PUBLIC hunspell)
target_compile_features(noctis_spelling PUBLIC cxx_std_23)
noctis_set_warnings(noctis_spelling)
