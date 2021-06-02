cmake_minimum_required(VERSION 3.18.0)
project(FerrumTestProject)

file(GLOB FE_TEST_PROJ_SOURCES "*.h" "*.cpp")
add_executable(FerrumTestProject ${FE_TEST_PROJ_SOURCES})
