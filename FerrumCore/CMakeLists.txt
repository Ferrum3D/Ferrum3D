cmake_minimum_required(VERSION 3.18.0)

project(FerrumCore)

set(SRC
	FeConsole.cpp
	StringUtils.cpp
	
	FeJobSystem.cpp
)
set(INCLUDE
	CoreUtils.h
	Platform.h
	
	FeVector.h
	FeHalf.h
	
	FeConsole.h
	FeLog.h
	StringUtils.h
	utf8h/utf8.h
	
	FeParallelQueue.h
	FeStackAllocator.h
	FeJobSystem.h
)

add_library(FerrumCore SHARED ${SRC} ${INCLUDE})

target_compile_definitions(FerrumCore PRIVATE FERRUMCORE_EXPORTS)
