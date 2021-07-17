cmake_minimum_required(VERSION 3.18.0)

project(FeCore)

set(SRC
	Assets/FeAssetManager.h
	Assets/FeAssetManager.cpp
	
	Console/FeConsole.h
	Console/FeConsole.cpp
	Console/FeLog.h
	
	Jobs/FeJobSystem.h
	Jobs/FeJobSystem.cpp
	Jobs/FeParallelQueue.h
	
	Math/FeHalf.h
	Math/FeVector3.h
	Math/FeVector4.h
	Math/FeVectorMath.h
	
	Memory/FeStackAllocator.h
	
	Scene/FeEntity.h
	Scene/FeScene.h
	
	Time/DateTime.h
	Time/DateTime.cpp
	Time/TimeSpan.h
	
	Utils/CoreUtils.h
	Utils/Platform.h
	Utils/Result.h
	Utils/StringUtils.h
	Utils/StringUtils.cpp
)

add_library(FeCore SHARED ${SRC} ${INCLUDE})

get_property("TARGET_SOURCE_FILES" TARGET FeCore PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

target_compile_definitions(FeCore PRIVATE FERRUMCORE_EXPORTS)
