cmake_minimum_required(VERSION 3.15)
project(coinc
        VERSION 1.0.1
        DESCRIPTION "Coinc, a program to seek coincidences from list-mode data"
        LANGUAGES C
)

configure_file(coinc_config.h.in coinc_config.h @ONLY)
add_executable(co