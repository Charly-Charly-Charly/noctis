cmake_minimum_required(VERSION 3.21)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

project(noctis
    VERSION 0.2.0
    DESCRIPTION "Editor de notas Markdown local-first"
    LANGUAGES CXX C
)

if(WIN32)
    enable_language(RC)
    # En algunos entornos el bin de MinGW no está en el PATH que usa el
    # lanzador de procesos del generador (aunque sí esté disponible al
    # invocarlo directo desde una shell): CMake entonces resuelve
    # CMAKE_RC_COMPILER como "windres" a secas, y windres.exe nunca aparece
    # al compilar el recurso del ícono. Se resuelve al lado del propio
    # compilador de C++, que si tiene ruta absoluta.
    if(MINGW AND CMAKE_RC_COMPILER STREQUAL "windres")
        get_filename_component(_mingw_bin_dir "${CMAKE_CXX_COMPILER}" DIRECTORY)
        find_program(_windres_full_path windres PATHS "${_mingw_bin_dir}" NO_DEFAULT_PATH)
        if(_windres_full_path)
            set(CMAKE_RC_COMPILER "${_windres_full_path}" CACHE FILEPATH "" FORCE)
        endif()
        unset(_mingw_bin_dir)
        unset(_windres_full_path)
    endif()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(CompilerWarnings)

option(NOCTIS_BUILD_TESTS "Compilar los tests unitarios" ON)
option(NOCTIS_BUILD_UI "Compilar la interfaz Qt (requiere Qt6)" ON)

include(FetchContent)

add_subdirectory(src)

if(NOCTIS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
