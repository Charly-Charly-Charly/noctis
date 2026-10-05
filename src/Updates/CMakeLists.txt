add_library(noctis_updates STATIC
    src/UpdateChecker.cpp
    include/Updates/UpdateChecker.h
)

target_include_directories(noctis_updates PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(noctis_updates PUBLIC Qt6::Network)
target_compile_features(noctis_updates PUBLIC cxx_std_23)
noctis_set_warnings(noctis_updates)

set_target_properties(noctis_updates PROPERTIES AUTOMOC ON)
