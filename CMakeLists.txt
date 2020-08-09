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

set(CPACK_PACKAGE_VERSION "${coinc_VERSION_MAJOR}.${coinc_VERSION_MINOR}.${coinc_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION_MAJOR "${coinc_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${coinc_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${coinc_VERSION_PATCH}")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
        "^${CMAKE_SOURCE_DIR}/.git/"
        "^${CMAKE_SOURCE_DIR}/\.idea/"
        ".gitignore$"
        ".DS_Store$"
        "^${CMAKE_SOURCE_DIR}/build/"
        "^${CMAKE_SOURCE_DIR}/cmake-build-.*/"
)
set(CPACK_SOURCE_PACKAGE_FILE_NAME "coinc-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
include(CPack)
