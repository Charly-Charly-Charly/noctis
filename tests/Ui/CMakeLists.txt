# PreviewWidget y Theme viven dentro del ejecutable `noctis` (no hay una
# librería de UI), así que el test compila esos dos .cpp directamente.
# Qt6::Svg (íconos) solo está declarado dentro de src/; acá hay que buscarlo de nuevo.
find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

add_executable(noctis_ui_tests
    PreviewPrintTests.cpp
    IconTests.cpp
    ThemeFontTests.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/src/IconButton.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/include/UI/IconButton.h
    ${CMAKE_SOURCE_DIR}/src/UI/src/TreeRowWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/include/UI/TreeRowWidget.h
    ${CMAKE_SOURCE_DIR}/src/UI/src/Icons.cpp
    ${CMAKE_SOURCE_DIR}/src/UI/include/UI/Icons.h
    ${CMAKE_SOURCE_DIR}/src/UI/resources/noctis.qrc
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
    Qt6::Svg
    GTest::gtest
)

target_compile_features(noctis_ui_tests PRIVATE cxx_std_23)
set_target_properties(noctis_ui_tests PROPERTIES AUTOMOC ON AUTORCC ON)

include(GoogleTest)
gtest_discover_tests(noctis_ui_tests
    DISCOVERY_MODE PRE_TEST
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
