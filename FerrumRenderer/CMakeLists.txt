cmake_minimum_required(VERSION 3.18.0)
project(FerrumRenderer)

file(GLOB FE_RENDERER_SOURCES "*.h" "*.cpp")
add_library(FerrumRenderer ${FE_RENDERER_SOURCES})
