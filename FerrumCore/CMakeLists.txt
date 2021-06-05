cmake_minimum_required(VERSION 3.18.0)

project(FerrumCore)

set(SRC
	src/FeConsole.cpp
	src/StringUtils.cpp
)
set(INCLUDE
	include/FeVector.h
	include/CoreUtils.h
	include/Platform.h
	include/FeConsole.h
	include/FeHalf.h
	include/FeLog.h
	include/StringUtils.h
	include/utf8h/utf8.h
)

add_library(FerrumCore SHARED ${SRC} ${INCLUDE})

target_include_directories(FerrumCore PRIVATE "include")
target_compile_definitions(FerrumCore PRIVATE FERRUMCORE_EXPORTS)
