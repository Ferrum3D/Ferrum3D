cmake_minimum_required(VERSION 3.18.0)

project(FerrumRenderer)

set(SRC
	src/FeRenderer.cpp
	src/FeWindow.cpp
)
set(INCLUDE
	include/FeRenderer.h
	include/FeWindow.h
)

add_library(FerrumRenderer SHARED ${SRC} ${INCLUDE})

target_include_directories(FerrumRenderer PRIVATE "include")
