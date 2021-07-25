cmake_minimum_required(VERSION 3.21.0)

set(SRC
	FeCore/Assets/FeAssetManager.h
	FeCore/Assets/FeAssetManager.cpp
	
	FeCore/Console/FeConsole.h
	FeCore/Console/FeConsole.cpp
	FeCore/Console/FeLog.h
	
	FeCore/Jobs/FeJobSystem.h
	FeCore/Jobs/FeJobSystem.cpp
	FeCore/Jobs/FeParallelQueue.h
	
	FeCore/Math/FeHalf.h
	FeCore/Math/FeVector3.h
	FeCore/Math/FeVector4.h
	FeCore/Math/FeVectorMath.h
	
	FeCore/Memory/BasicSystemAllocator.h
	FeCore/Memory/FeStackAllocator.h
	FeCore/Memory/IBasicAllocator.h
	
	FeCore/Modules/Environment.h
	FeCore/Modules/Environment.cpp
	
	FeCore/Scene/FeEntity.h
	FeCore/Scene/FeScene.h
	
	FeCore/Time/DateTime.h
	FeCore/Time/DateTime.cpp
	FeCore/Time/TimeSpan.h
	
	FeCore/Utils/CoreUtils.h
	FeCore/Utils/Platform.h
	FeCore/Utils/Result.h
	FeCore/Utils/SortedStringVector.h
	FeCore/Utils/StringUtils.h
	FeCore/Utils/StringUtils.cpp
)

add_library(FeCore SHARED ${SRC})

get_property("TARGET_SOURCE_FILES" TARGET FeCore PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

target_compile_definitions(FeCore PRIVATE FERRUMCORE_EXPORTS)
target_compile_definitions(FeCore PUBLIC _ENABLE_EXTENDED_ALIGNED_STORAGE)

add_subdirectory(Tests)
