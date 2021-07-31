cmake_minimum_required(VERSION 3.21.0)

set(SRC
	FeCore/Assets/AssetManager.h
	FeCore/Assets/AssetManager.cpp
	
	FeCore/Console/Console.h
	FeCore/Console/Console.cpp
	FeCore/Console/FeLog.h
	
	FeCore/Jobs/JobSystem.h
	FeCore/Jobs/JobSystem.cpp
	FeCore/Jobs/ParallelQueue.h
	
	FeCore/Math/Half.h
	FeCore/Math/Vector3.h
	FeCore/Math/Vector4.h
	FeCore/Math/VectorMath.h
	
	FeCore/Memory/Allocator.h
	FeCore/Memory/AllocatorBase.h
	FeCore/Memory/AllocatorBase.cpp
	FeCore/Memory/BasicSystemAllocator.h
	FeCore/Memory/HeapAllocator.h
	FeCore/Memory/HeapAllocator.cpp
	FeCore/Memory/IAllocator.h
	FeCore/Memory/StackAllocator.h
	FeCore/Memory/IBasicAllocator.h
	
	FeCore/Modules/Environment.h
	FeCore/Modules/Environment.cpp
	FeCore/Modules/Singleton.h
	
	FeCore/Scene/Entity.h
	FeCore/Scene/Scene.h
	
	FeCore/SIMD/Scalar/Float32x4.h
	FeCore/SIMD/Scalar/Int32x4.h
	FeCore/SIMD/SSE/Float32x4.h
	FeCore/SIMD/SSE/Int32x4.h
	FeCore/SIMD/CommonSIMD.h
	FeCore/SIMD/SIMDIntrin.h
	
	FeCore/Strings/FeUnicode.h
	FeCore/Strings/Format.h
	FeCore/Strings/Format.cpp
	FeCore/Strings/String.h
	FeCore/Strings/StringSlice.h
	
	FeCore/Time/DateTime.h
	FeCore/Time/DateTime.cpp
	FeCore/Time/TimeSpan.h
	
	FeCore/Utils/CoreUtils.h
	FeCore/Utils/Platform.h
	FeCore/Utils/Result.h
	FeCore/Utils/SortedStringVector.h
)

add_library(FeCore STATIC ${SRC})

get_property("TARGET_SOURCE_FILES" TARGET FeCore PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

target_compile_definitions(FeCore PRIVATE FERRUMCORE_EXPORTS)
target_compile_definitions(FeCore PUBLIC _ENABLE_EXTENDED_ALIGNED_STORAGE)

add_subdirectory(Tests)
