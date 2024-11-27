set(PUBLIC_HEADERS
    Public/FeCore/Assets/Asset.h
    Public/FeCore/Assets/AssetCommon.h
    Public/FeCore/Assets/AssetStorage.h
    Public/FeCore/Assets/IAssetLoader.h
    Public/FeCore/Assets/IAssetManager.h
    Public/FeCore/Assets/WeakAsset.h

    Public/FeCore/Base/Base.h
    Public/FeCore/Base/BaseMath.h
    Public/FeCore/Base/CompilerTraits.h
    Public/FeCore/Base/Hash.h
    Public/FeCore/Base/Platform.h
    Public/FeCore/Base/PlatformInclude.h
    Public/FeCore/Base/PlatformTraits.h

    Public/FeCore/Components/LocalToWorldComponent.h
    Public/FeCore/Components/PositionComponent.h

    Public/FeCore/Console/Console.h

    Public/FeCore/Containers/ByteBuffer.h
    Public/FeCore/Containers/HashTables.h
    Public/FeCore/Containers/LRUCacheMap.h
    Public/FeCore/Containers/SegmentedVector.h
    Public/FeCore/Containers/SmallVector.h
    Public/FeCore/Containers/SparseSet.h
    Public/FeCore/Containers/SparseStorage.h

    Public/FeCore/DI/Activator.h
    Public/FeCore/DI/BaseDI.h
    Public/FeCore/DI/Builder.h
    Public/FeCore/DI/Container.h
    Public/FeCore/DI/LifetimeScope.h
    Public/FeCore/DI/Registration.h
    Public/FeCore/DI/Registry.h

    Public/FeCore/ECS/ECS.h

    Public/FeCore/EventBus/CoreEvents.h
    Public/FeCore/EventBus/EventBus.h

    Public/FeCore/Framework/ApplicationModule.h
    Public/FeCore/Framework/ModuleBase.h
    Public/FeCore/Framework/IModule.h

    Public/FeCore/IO/IAsyncStreamIO.h
    Public/FeCore/IO/BaseIO.h
    Public/FeCore/IO/FileStream.h
    Public/FeCore/IO/IStream.h
    Public/FeCore/IO/StreamBase.h
    Public/FeCore/IO/IStreamFactory.h

    Public/FeCore/Jobs/IJobSystem.h
    Public/FeCore/Jobs/Job.h
    Public/FeCore/Jobs/JobSystem.h
    Public/FeCore/Jobs/WaitGroup.h

    Public/FeCore/Logging/Logger.h
    Public/FeCore/Logging/Trace.h

    Public/FeCore/Math/Color.h
    Public/FeCore/Math/Colors.h
    Public/FeCore/Math/Half.h
    Public/FeCore/Math/Matrix4x4F.h
    Public/FeCore/Math/Quaternion.h
    Public/FeCore/Math/Vector2.h
    Public/FeCore/Math/Vector3.h
    Public/FeCore/Math/Vector4.h
    Public/FeCore/Math/VectorMath.h
    
    Public/FeCore/Memory/LinearAllocator.h
    Public/FeCore/Memory/Memory.h
    Public/FeCore/Memory/NullableHandle.h
    Public/FeCore/Memory/PoolAllocator.h
    Public/FeCore/Memory/RefCount.h

    Public/FeCore/Modules/Configuration.h
    Public/FeCore/Modules/Environment.h
    Public/FeCore/Modules/EnvironmentPrivate.h
    Public/FeCore/Modules/LibraryLoader.h
    Public/FeCore/Modules/ServiceLocator.h

    Public/FeCore/Parallel/Context.h
    Public/FeCore/Parallel/Event.h
    Public/FeCore/Parallel/Fiber.h
    Public/FeCore/Parallel/Mutex.h
    Public/FeCore/Parallel/Semaphore.h
    Public/FeCore/Parallel/SpinLock.h
    Public/FeCore/Parallel/Thread.h

    Public/FeCore/RTTI/RTTI.h

    Public/FeCore/SIMD/CommonSIMD.h
    Public/FeCore/SIMD/SIMDIntrin.h

    Public/FeCore/SIMD/SSE/Float32x4.h
    Public/FeCore/SIMD/SSE/Int32x4.h

    Public/FeCore/SIMD/Scalar/Float32x4.h
    Public/FeCore/SIMD/Scalar/Int32x4.h

    Public/FeCore/Strings/FixedString.h
    Public/FeCore/Strings/Unicode.h
    Public/FeCore/Strings/Format.h
    Public/FeCore/Strings/String.h
    Public/FeCore/Strings/StringBase.h
    Public/FeCore/Strings/StringSlice.h

    Public/FeCore/Time/BaseTime.h
    Public/FeCore/Time/DateTime.h
    Public/FeCore/Time/TimeSpan.h

    Public/FeCore/Utils/BinarySerializer.h
    Public/FeCore/Utils/BoolPointer.h
    Public/FeCore/Utils/Result.h
    Public/FeCore/Utils/UUID.h
)

set(COMMON_SOURCES
    Private/FeCore/Assets/AssetManager.h
    Private/FeCore/Assets/AssetManager.cpp

    Private/FeCore/Console/Console.cpp

    Private/FeCore/DI/BaseDI.cpp
    Private/FeCore/DI/Builder.cpp
    Private/FeCore/DI/Container.cpp
    Private/FeCore/DI/LifetimeScope.cpp

    Private/FeCore/ECS/ECS.cpp

    Private/FeCore/Framework/ApplicationModule.cpp
    Private/FeCore/Framework/ModuleBase.cpp

    Private/FeCore/IO/Platform/PlatformFile.h
    Private/FeCore/IO/AsyncStreamIO.h
    Private/FeCore/IO/AsyncStreamIO.cpp
    Private/FeCore/IO/BaseIO.cpp
    Private/FeCore/IO/FileStream.cpp
    Private/FeCore/IO/StreamFactory.h
    Private/FeCore/IO/StreamFactory.cpp

    Private/FeCore/Jobs/JobSystem.cpp
    Private/FeCore/Jobs/WaitGroup.cpp

    Private/FeCore/Logging/Logger.cpp

    Private/FeCore/Math/Colors.cpp

    Private/FeCore/Memory/LinearAllocator.cpp
    Private/FeCore/Memory/Memory.cpp
    Private/FeCore/Memory/PoolAllocator.cpp

    Private/FeCore/Modules/Configuration.cpp
    Private/FeCore/Modules/Environment.cpp
    Private/FeCore/Modules/LibraryLoader.cpp

    Private/FeCore/Parallel/Context.cpp
    Private/FeCore/Parallel/Fiber.cpp

    Private/FeCore/Platform/Windows/Common.h

    Private/FeCore/Strings/Format.cpp
    Private/FeCore/Strings/StringSlice.cpp

    Private/FeCore/Time/Platform/PlatformTime.cpp
    Private/FeCore/Time/DateTime.cpp
)

set(WINDOWS_SOURCES
    Private/FeCore/IO/Platform/Windows/PlatformFile.cpp

    Private/FeCore/Logging/Platform/Windows/Trace.cpp

    Private/FeCore/Parallel/Platform/Windows/Event.cpp
    Private/FeCore/Parallel/Platform/Windows/Thread.cpp
    Private/FeCore/Parallel/Platform/Windows/Mutex.cpp
    Private/FeCore/Parallel/Platform/Windows/Semaphore.cpp
    
    Private/FeCore/Time/Platform/Windows/PlatformTime.cpp
)


add_library(FeCore STATIC ${PUBLIC_HEADERS} ${COMMON_SOURCES} ${WINDOWS_SOURCES})

fe_configure_target(FeCore)
fe_enable_sse_for_target(FeCore)

target_include_directories(FeCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FeCore PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")

set_target_properties(FeCore PROPERTIES FOLDER "Engine/FeCore")
target_link_libraries(FeCore
    fcontext mimalloc EASTL unordered_dense small_vector
    TracyClient jeaiii_itoa dragonbox::dragonbox_to_chars)

get_property("TARGET_SOURCE_FILES" TARGET FeCore PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

add_subdirectory(Tests)
