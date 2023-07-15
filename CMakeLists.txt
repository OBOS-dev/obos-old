cmake_minimum_required(VERSION 3.8)

project(obos C CXX ASM-ATT)

add_subdirectory("kernel")
add_subdirectory("fatDriver")
add_subdirectory("launch-qemu")