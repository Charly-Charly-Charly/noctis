add_executable(noctis_filesystem_tests
    FrontmatterTests.cpp
    NoteIdentityTests.cpp
    LegacyIdMigrationTests.cpp
    SyncServiceTests.cpp
)

target_link_libraries(noctis_filesystem_tests PRIVATE
    noctis_filesystem
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(noctis_filesystem_tests)
