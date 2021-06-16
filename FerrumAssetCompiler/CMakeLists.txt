cmake_minimum_required(VERSION 3.18.0)

project(FerrumAssetCompiler)

set(SRC
	main.cpp
)
set(INCLUDE)

add_executable(FerrumAssetCompiler ${SRC} ${INCLUDE})
