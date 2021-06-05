cmake_minimum_required(VERSION 3.18.0)

project(FerrumTestProject)

set(SRC
	src/main.cpp
)
set(INCLUDE)

add_executable(FerrumTestProject ${SRC} ${INCLUDE})

target_include_directories(FerrumTestProject PRIVATE "include")
