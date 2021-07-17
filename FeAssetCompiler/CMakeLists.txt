cmake_minimum_required(VERSION 3.18.0)

project(FeAssetCompiler)

set(SRC
	main.cpp
)
set(INCLUDE)

add_executable(FeAssetCompiler ${SRC} ${INCLUDE})
