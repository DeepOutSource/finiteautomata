cmake_minimum_required(VERSION 3.15)
project(coinc
        VERSION 1.0.1
        DESCRIPTION "Coinc, a program to seek coincidences from list-mode data"
        LANGUAGES C
)

configure_file(coinc_config.h.in coinc_config.h @ONLY)
add_executable(coinc coinc.c)
target_include_directories(coinc PRIVATE
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>)
install(TARGETS coinc RUNTIME DESTINATION bin)

set(CPACK_PACKAGE_VERSION "${coinc_VERSION_MAJOR}.${coinc_VERSION_MINOR}.${coinc_VERSIO