# PreviewWidget y Theme viven dentro del ejecutable `noctis` (no hay una
# librería de UI), así que el test compila esos dos .cpp directamente.
add_executable(noctis_ui_tests
    PreviewPrintTests.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/src/PreviewWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/include/UI/PreviewWidget.h
    ${CMAKE_SOURCE_DIR}/src/UI/src/Theme.cpp
)

target_include_directories(noctis_ui_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/UI/include
)

target_link_libraries(noctis_ui_tests PRIVATE
    noctis_markdown
    noctis_editor
    GTest::gtest
)

target_compile_features(noctis_ui_tests PRIVATE cxx_std_23)
set_target_properties(noctis_ui_tests PROPERTIES AUTOMOC ON)

include(GoogleTest)
gtest_discover_tests(noctis_ui_tests
    DISCOVERY_MODE PRE_TEST
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
