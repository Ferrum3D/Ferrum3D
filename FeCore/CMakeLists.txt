set(SRC
	FeCore/Console/Console.h
	FeCore/Console/Console.cpp
	FeCore/Console/ConsoleLogger.h
	FeCore/Console/ConsoleLogger.cpp
	FeCore/Console/FeLog.h
	FeCore/Console/IConsoleLogger.h
	
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
	FeCore/Memory/IBasicAllocator.h
	FeCore/Memory/Memory.h
	FeCore/Memory/SharedPtr.h
	FeCore/Memory/StackAllocator.h
	
	FeCore/Modules/Environment.h
	FeCore/Modules/Environment.cpp
	FeCore/Modules/Singleton.h

	FeCore/RTTI/RTTI.h
	
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
	FeCore/Utils/Interlocked.h
	FeCore/Utils/Interlocked.cpp
	FeCore/Utils/Platform.h
	FeCore/Utils/Result.h
	FeCore/Utils/SortedStringVector.h
)

add_library(FeCore STATIC ${SRC})

fe_enable_sse_for_target(FeCore)

target_include_directories(FeCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(FeCore rpmalloc entt)

get_property("TARGET_SOURCE_FILES" TARGET FeCore PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

add_subdirectory(Tests)
