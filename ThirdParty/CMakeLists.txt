cmake_minimum_required(VERSION 3.18.0)

set(CMAKE_CXX_STANDARD 14)

project(ThirdParty)
add_subdirectory("glfw")
add_subdirectory("DiligentEngine")

set(CMAKE_CXX_STANDARD 17)
