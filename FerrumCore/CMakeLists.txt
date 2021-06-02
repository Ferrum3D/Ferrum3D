cmake_minimum_required(VERSION 3.18.0)
project(FerrumCore)

file(GLOB FE_CORE_SOURCES "*.h" "*.cpp")
add_library(FerrumCore ${FE_CORE_SOURCES})
