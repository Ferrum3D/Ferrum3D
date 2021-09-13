set(SRC
    FeCore/Base/Base.h
    FeCore/Base/CompilerTraits.h
    FeCore/Base/Platform.h
    FeCore/Base/PlatformInclude.h
    FeCore/Base/PlatformTraits.h

    FeCore/Console/Console.h
    FeCore/Console/Console.cpp
    FeCore/Console/ConsoleLogger.h
    FeCore/Console/ConsoleLogger.cpp
    FeCore/Console/FeLog.h
    FeCore/Console/IConsoleLogger.h

    FeCore/Containers/ArraySlice.h

    FeCore/EventBus/EventBus.h

    FeCore/IO/BaseIO.h
    FeCore/IO/BaseIO.cpp
    FeCore/IO/FileHandle.h
    FeCore/IO/FileHandle.cpp
    FeCore/IO/FileStream.h
    FeCore/IO/FileStream.cpp
    FeCore/IO/IStream.h
    FeCore/IO/StdoutStream.h
    FeCore/IO/StdoutStream.cpp
    FeCore/IO/StreamBase.h

    FeCore/Math/Color.h
    FeCore/Math/Colors.h
    FeCore/Math/Colors.cpp
    FeCore/Math/Half.h
    FeCore/Math/MathUtils.h
    FeCore/Math/Matrix4x4F.h
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
    FeCore/Memory/Object.h
    FeCore/Memory/ReferenceCounter.h
    FeCore/Memory/SharedPtr.h
    FeCore/Memory/StackAllocator.h
    FeCore/Memory/UniquePtr.h

    FeCore/Modules/DynamicLibrary.h
    FeCore/Modules/DynamicLibrary.cpp
    FeCore/Modules/Environment.h
    FeCore/Modules/Environment.cpp
    FeCore/Modules/Singleton.h

    FeCore/Parallel/Interlocked.h
    FeCore/Parallel/Interlocked.cpp
    FeCore/Parallel/Locker.h
    FeCore/Parallel/Mutex.h
    FeCore/Parallel/Mutex.cpp
    FeCore/Parallel/SpinMutex.h

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

    FeCore/Utils/Result.h
    FeCore/Utils/SortedStringVector.h
    FeCore/Utils/UUID.h

    FeCore/Assets/AssetStorage.h
    FeCore/Assets/Asset.h
    FeCore/Assets/WeakAsset.h
    FeCore/Assets/IAssetLoader.h
    FeCore/Assets/IAssetProvider.h
    FeCore/Assets/AssetRegistry.h
    FeCore/Assets/AssetRegistry.cpp
    FeCore/Assets/AssetProviderDev.h
    FeCore/Assets/AssetProviderDev.cpp
    FeCore/Assets/IAssetManager.h
    FeCore/Assets/AssetCommon.h
    FeCore/Assets/AssetManager.h
    FeCore/Assets/AssetManager.cpp
    FeCore/Jobs/JobTree.h
    FeCore/Parallel/AtomicFlags.h
    FeCore/Jobs/Job.h
    FeCore/Utils/BoolPointer.h
    FeCore/Jobs/IJobScheduler.h
    FeCore/Jobs/JobScheduler.h
    FeCore/Jobs/JobScheduler.cpp
    FeCore/Parallel/Semaphore.h
    FeCore/Parallel/Semaphore.cpp
    FeCore/Allocators/MonotonicAllocator.h
    FeCore/EventBus/FrameEvents.h)

add_library(FeCore STATIC ${SRC})

fe_configure_target(FeCore)
fe_enable_sse_for_target(FeCore)

target_include_directories(FeCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(FeCore PROPERTIES FOLDER "Engine/FeCore")
target_link_libraries(FeCore rpmalloc entt)

get_property("TARGET_SOURCE_FILES" TARGET FeCore PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

add_subdirectory(Tests)
