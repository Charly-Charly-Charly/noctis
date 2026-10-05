add_executable(noctis_editor_tests
    EditorWidgetTests.cpp
)

target_link_libraries(noctis_editor_tests PRIVATE
    noctis_editor
    GTest::gtest
)

include(GoogleTest)
# Sin pantalla real: el plugin "offscreen" de Qt alcanza para layout y teclado.
# PRE_TEST: el descubrimiento corre al lanzar ctest y no al compilar, porque
# el ejecutable necesita las DLL de Qt/MinGW en el PATH y eso no está
# garantizado dentro del paso de build.
gtest_discover_tests(noctis_editor_tests
    DISCOVERY_MODE PRE_TEST
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
