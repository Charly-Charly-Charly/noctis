add_executable(noctis_updates_tests
    UpdateCheckerTests.cpp
)

target_link_libraries(noctis_updates_tests PRIVATE
    noctis_updates
    GTest::gtest
)

include(GoogleTest)
gtest_discover_tests(noctis_updates_tests DISCOVERY_MODE PRE_TEST)
