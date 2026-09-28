add_executable(noctis_core_tests
    NoteServiceTests.cpp
)

target_link_libraries(noctis_core_tests PRIVATE
    noctis_core
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(noctis_core_tests)
