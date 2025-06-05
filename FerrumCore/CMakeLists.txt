set(PUBLIC_HEADERS
    Public/festd/Internal/BitStorage.h
    Public/festd/Internal/StringBase.h
    Public/festd/Internal/StringStorageImpl.h

    Public/festd/base.h
    Public/festd/bit_vector.h
    Public/festd/fixed_function.h
    Public/festd/intrusive_list.h
    Public/festd/ring_buffer.h
    Public/festd/span.h
    Public/festd/string.h
    Public/festd/unordered_map.h
    Public/festd/vector.h

    Public/FeCore/Assets/Asset.h
    Public/FeCore/Assets/AssetStorage.h
    Public/FeCore/Assets/IAssetLoader.h
    Public/FeCore/Assets/IAssetManager.h
    Public/FeCore/Assets/WeakAsset.h

    Public/FeCore/Base/Assert.h
    Public/FeCore/Base/Base.h
    Public/FeCore/Base/BaseMath.h
    Public/FeCore/Base/BaseTypes.h
    Public/FeCore/Base/CompilerTraits.h
    Public/FeCore/Base/Hash.h
    Public/FeCore/Base/Platform.h
    Public/FeCore/Base/PlatformInclude.h
    Public/FeCore/Base/PlatformTraits.h

    Public/FeCore/Console/Console.h
    Public/FeCore/Console/ConsolePrivate.h

    Public/FeCore/Containers/ByteBuffer.h
    Public/FeCore/Containers/ConcurrentQueue.h
    Public/FeCore/Containers/LRUCacheMap.h
    Public/FeCore/Containers/SegmentedVector.h

    Public/FeCore/DI/Activator.h
    Public/FeCore/DI/BaseDI.h
    Public/FeCore/DI/Builder.h
    Public/FeCore/DI/Container.h
    Public/FeCore/DI/LifetimeScope.h
    Public/FeCore/DI/Registration.h
    Public/FeCore/DI/Registry.h

    Public/FeCore/IO/IAsyncStreamIO.h
    Public/FeCore/IO/BaseIO.h
    Public/FeCore/IO/FileStream.h
    Public/FeCore/IO/IStream.h
    Public/FeCore/IO/IStreamFactory.h
    Public/FeCore/IO/Path.h
    Public/FeCore/IO/PathParser.h
    Public/FeCore/IO/StreamBase.h

    Public/FeCore/Jobs/IJobSystem.h
    Public/FeCore/Jobs/Job.h
    Public/FeCore/Jobs/JobSystem.h
    Public/FeCore/Jobs/WaitGroup.h

    Public/FeCore/Logging/Logger.h
    Public/FeCore/Logging/Trace.h

    Public/FeCore/Math/Aabb.h
    Public/FeCore/Math/Color.h
    Public/FeCore/Math/Colors.h
    Public/FeCore/Math/Matrix4x4F.h
    Public/FeCore/Math/Quaternion.h
    Public/FeCore/Math/Random.h
    Public/FeCore/Math/Rect.h
    Public/FeCore/Math/UUID.h
    Public/FeCore/Math/Vector2.h
    Public/FeCore/Math/Vector3.h
    Public/FeCore/Math/Vector3Int.h
    Public/FeCore/Math/Vector3UInt.h
    Public/FeCore/Math/Vector4.h
    Public/FeCore/Math/VectorMath.h

    Public/FeCore/Memory/FiberTempAllocator.h
    Public/FeCore/Memory/LinearAllocator.h
    Public/FeCore/Memory/Memory.h
    Public/FeCore/Memory/PoolAllocator.h
    Public/FeCore/Memory/RefCount.h
    Public/FeCore/Memory/SegmentedBuffer.h

    Public/FeCore/Modules/ApplicationModule.h
    Public/FeCore/Modules/Configuration.h
    Public/FeCore/Modules/Environment.h
    Public/FeCore/Modules/EnvironmentPrivate.h
    Public/FeCore/Modules/IModule.h
    Public/FeCore/Modules/LibraryLoader.h
    Public/FeCore/Modules/ModuleBase.h
    Public/FeCore/Modules/ServiceLocator.h

    Public/FeCore/RTTI/RTTI.h

    Public/FeCore/SIMD/Utils.h

    Public/FeCore/Strings/Encoding.h
    Public/FeCore/Strings/Format.h
    Public/FeCore/Strings/Parser.h
    Public/FeCore/Strings/Utils.h

    Public/FeCore/Threading/Context.h
    Public/FeCore/Threading/Event.h
    Public/FeCore/Threading/Fiber.h
    Public/FeCore/Threading/Mutex.h
    Public/FeCore/Threading/Semaphore.h
    Public/FeCore/Threading/SharedSpinLock.h
    Public/FeCore/Threading/SpinLock.h
    Public/FeCore/Threading/Thread.h

    Public/FeCore/Time/BaseTime.h
    Public/FeCore/Time/DateTime.h
    Public/FeCore/Time/TimeSpan.h
)

set(COMMON_SOURCES
    Private/FeCore/Assets/AssetManager.h
    Private/FeCore/Assets/AssetManager.cpp

    Private/FeCore/Base/Assert.cpp

    Private/FeCore/Console/Console.cpp

    Private/FeCore/DI/BaseDI.cpp
    Private/FeCore/DI/Builder.cpp
    Private/FeCore/DI/Container.cpp
    Private/FeCore/DI/LifetimeScope.cpp

    Private/FeCore/IO/Platform/PlatformFile.h
    Private/FeCore/IO/Platform/PlatformPath.h
    Private/FeCore/IO/AsyncStreamIO.h
    Private/FeCore/IO/AsyncStreamIO.cpp
    Private/FeCore/IO/BaseIO.cpp
    Private/FeCore/IO/FileStream.cpp
    Private/FeCore/IO/Path.cpp
    Private/FeCore/IO/StreamFactory.h
    Private/FeCore/IO/StreamFactory.cpp

    Private/FeCore/Jobs/JobSystem.cpp
    Private/FeCore/Jobs/WaitGroup.cpp

    Private/FeCore/Logging/Logger.cpp

    Private/FeCore/Math/Colors.cpp
    Private/FeCore/Math/Matrix4x4F.cpp

    Private/FeCore/Memory/FiberTempAllocator.cpp
    Private/FeCore/Memory/LinearAllocator.cpp
    Private/FeCore/Memory/Memory.cpp
    Private/FeCore/Memory/PoolAllocator.cpp
    Private/FeCore/Memory/tlsf.c
    Private/FeCore/Memory/tlsf.h

    Private/FeCore/Modules/ApplicationModule.cpp
    Private/FeCore/Modules/Configuration.cpp
    Private/FeCore/Modules/Environment.cpp
    Private/FeCore/Modules/LibraryLoader.cpp
    Private/FeCore/Modules/ModuleBase.cpp

    Private/FeCore/Platform/Windows/Common.h
    Private/FeCore/Platform/Windows/Platform.cpp

    Private/FeCore/Strings/Encoding.cpp
    Private/FeCore/Strings/Format.cpp
    Private/FeCore/Strings/Parser.cpp

    Private/FeCore/Threading/Context.cpp
    Private/FeCore/Threading/Fiber.cpp

    Private/FeCore/Time/Platform/PlatformTime.cpp
    Private/FeCore/Time/DateTime.cpp
)

set(WINDOWS_SOURCES
    Private/FeCore/Base/Platform/Windows/PlatformAssert.cpp

    Private/FeCore/IO/Platform/Windows/PlatformFile.cpp
    Private/FeCore/IO/Platform/Windows/PlatformPath.cpp

    Private/FeCore/Threading/Platform/Windows/Event.cpp
    Private/FeCore/Threading/Platform/Windows/Thread.cpp
    Private/FeCore/Threading/Platform/Windows/Mutex.cpp
    Private/FeCore/Threading/Platform/Windows/Semaphore.cpp

    Private/FeCore/Time/Platform/Windows/PlatformTime.cpp
)


add_library(FeCore STATIC ${PUBLIC_HEADERS} ${COMMON_SOURCES} ${WINDOWS_SOURCES})

fe_configure_target(FeCore)

target_include_directories(FeCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FeCore PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")

set_target_properties(FeCore PROPERTIES FOLDER "Core")
target_link_libraries(FeCore
    fcontext
    mimalloc
    TracyClient
    utf8proc
    EASTL
    unordered_dense
    small_vector
    tl_expected
    jeaiii_itoa
    dragonbox::dragonbox_to_chars)

get_property("TARGET_SOURCE_FILES" TARGET FeCore PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

add_subdirectory(Tests)
