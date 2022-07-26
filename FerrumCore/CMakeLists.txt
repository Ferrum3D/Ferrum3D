set(SRC
    # FERRUM_SORT_LINES BEGIN
    FeCore/Allocators/LinearAllocator.h
    FeCore/Allocators/MonotonicAllocator.h

    FeCore/Assets/Asset.h
    FeCore/Assets/AssetCommon.h
    FeCore/Assets/AssetManager.cpp
    FeCore/Assets/AssetManager.h
    FeCore/Assets/AssetProviderDev.cpp
    FeCore/Assets/AssetProviderDev.h
    FeCore/Assets/AssetRegistry.cpp
    FeCore/Assets/AssetRegistry.h
    FeCore/Assets/AssetStorage.h
    FeCore/Assets/IAssetLoader.h
    FeCore/Assets/IAssetManager.h
    FeCore/Assets/IAssetProvider.h
    FeCore/Assets/WeakAsset.h

    FeCore/Base/Base.h
    FeCore/Base/CompilerTraits.h
    FeCore/Base/Platform.h
    FeCore/Base/PlatformInclude.h
    FeCore/Base/PlatformTraits.h

    FeCore/Components/LocalToWorldComponent.h
    FeCore/Components/PositionComponent.h

    FeCore/Console/Console.cpp
    FeCore/Console/Console.h
    FeCore/Console/ConsoleLogger.cpp
    FeCore/Console/ConsoleLogger.h
    FeCore/Console/FeLog.h
    FeCore/Console/IConsoleLogger.h

    FeCore/Containers/ArraySlice.h
    FeCore/Containers/ArraySliceMut.h
    FeCore/Containers/ByteBuffer.h
    FeCore/Containers/LRUCacheMap.h
    FeCore/Containers/List.h
    FeCore/Containers/SparseSet.h
    FeCore/Containers/SparseStorage.h

    FeCore/ECS/ArchetypeChunk.cpp
    FeCore/ECS/ArchetypeChunk.h
    FeCore/ECS/ComponentStorage.cpp
    FeCore/ECS/ComponentStorage.h
    FeCore/ECS/ComponentSystem.h
    FeCore/ECS/ComponentType.h
    FeCore/ECS/ECSCommon.h
    FeCore/ECS/Entity.h
    FeCore/ECS/EntityArchetype.cpp
    FeCore/ECS/EntityArchetype.h
    FeCore/ECS/EntityQuery.h
    FeCore/ECS/EntityRegistry.cpp
    FeCore/ECS/EntityRegistry.h
    FeCore/ECS/ISystem.h
    FeCore/ECS/World.cpp
    FeCore/ECS/World.h

    FeCore/EventBus/EventBus.h
    FeCore/EventBus/FrameEvents.h

    FeCore/Framework/ApplicationFramework.cpp
    FeCore/Framework/ApplicationFramework.h
    FeCore/Framework/FrameworkBase.cpp
    FeCore/Framework/FrameworkBase.h
    FeCore/Framework/IFramework.h
    FeCore/Framework/ModuleFramework.h

    FeCore/IO/BaseIO.cpp
    FeCore/IO/BaseIO.h
    FeCore/IO/FileHandle.cpp
    FeCore/IO/FileHandle.h
    FeCore/IO/FileStream.cpp
    FeCore/IO/FileStream.h
    FeCore/IO/IStream.h
    FeCore/IO/StdoutStream.cpp
    FeCore/IO/StdoutStream.h
    FeCore/IO/StreamBase.h

    FeCore/Jobs/IJobScheduler.h
    FeCore/Jobs/Job.h
    FeCore/Jobs/JobScheduler.cpp
    FeCore/Jobs/JobScheduler.h
    FeCore/Jobs/JobTree.h
    FeCore/Jobs/SignalJob.h

    FeCore/Math/Color.h
    FeCore/Math/Colors.cpp
    FeCore/Math/Colors.h
    FeCore/Math/Half.h
    FeCore/Math/MathUtils.h
    FeCore/Math/Matrix4x4F.h
    FeCore/Math/Quaternion.h
    FeCore/Math/Vector2.h
    FeCore/Math/Vector3.h
    FeCore/Math/Vector4.h
    FeCore/Math/VectorMath.h

    FeCore/Memory/Allocator.h
    FeCore/Memory/AllocatorBase.cpp
    FeCore/Memory/AllocatorBase.h
    FeCore/Memory/BasicSystemAllocator.h
    FeCore/Memory/HeapAllocator.cpp
    FeCore/Memory/HeapAllocator.h
    FeCore/Memory/IAllocator.h
    FeCore/Memory/IBasicAllocator.h
    FeCore/Memory/Memory.h
    FeCore/Memory/NullableHandle.h
    FeCore/Memory/Object.h
    FeCore/Memory/ReferenceCounter.h
    FeCore/Memory/SharedPtr.h
    FeCore/Memory/StackAllocator.h
    FeCore/Memory/UniquePtr.h

    FeCore/Modules/DynamicLibrary.cpp
    FeCore/Modules/DynamicLibrary.h
    FeCore/Modules/Environment.cpp
    FeCore/Modules/Environment.h
    FeCore/Modules/SharedInterface.h

    FeCore/Parallel/AtomicFlags.h
    FeCore/Parallel/Interlocked.cpp
    FeCore/Parallel/Interlocked.h
    FeCore/Parallel/Locker.h
    FeCore/Parallel/Mutex.cpp
    FeCore/Parallel/Mutex.h
    FeCore/Parallel/Semaphore.cpp
    FeCore/Parallel/Semaphore.h
    FeCore/Parallel/SpinMutex.h

    FeCore/RTTI/RTTI.h

    FeCore/SIMD/CommonSIMD.h
    FeCore/SIMD/SIMDIntrin.h

    FeCore/SIMD/SSE/Float32x4.h
    FeCore/SIMD/SSE/Int32x4.h

    FeCore/SIMD/Scalar/Float32x4.h
    FeCore/SIMD/Scalar/Int32x4.h

    FeCore/Strings/FeUnicode.h
    FeCore/Strings/Format.cpp
    FeCore/Strings/Format.h
    FeCore/Strings/String.h
    FeCore/Strings/StringSlice.cpp
    FeCore/Strings/StringSlice.h

    FeCore/Systems/CallbackSystem.cpp
    FeCore/Systems/CallbackSystem.h
    FeCore/Systems/TransformSystem.cpp
    FeCore/Systems/TransformSystem.h

    FeCore/Time/DateTime.cpp
    FeCore/Time/DateTime.h
    FeCore/Time/TimeSpan.h

    FeCore/Utils/BoolPointer.h
    FeCore/Utils/Result.h
    FeCore/Utils/SortedStringVector.h
    FeCore/Utils/UUID.h
    # FERRUM_SORT_LINES END
)

add_library(FeCore STATIC ${SRC})

fe_configure_target(FeCore)
fe_enable_sse_for_target(FeCore)

target_include_directories(FeCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(FeCore PROPERTIES FOLDER "Engine/FeCore")
target_link_libraries(FeCore rpmalloc entt)

get_property("TARGET_SOURCE_FILES" TARGET FeCore PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

add_subdirectory(Tests)
add_subdirectory(Bindings)
