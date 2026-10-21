cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(panel_hafajstos)

include_directories(src/chainbus_hat_protocol)

target_sources(app PRIVATE src/main.c src/chainbus_hat_protocol/iG5A.c )