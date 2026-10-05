include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_subdirectory(Core)
add_subdirectory(Filesystem)

if(NOCTIS_BUILD_UI)
    add_subdirectory(Editor)
    add_subdirectory(Ui)
endif()
