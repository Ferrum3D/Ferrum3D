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

    Public/Core/Base/Assert.h
    Public/Core/Base/Base.h
    Public/Core/Base/BaseMath.h
    Public/Core/Base/BaseTypes.h
    Public/Core/Base/CompilerTraits.h
    Public/Core/Base/Hash.h
    Public/Core/Base/Platform.h
    Public/Core/Base/PlatformInclude.h
    Public/Core/Base/PlatformTraits.h
    Public/Core/Base/StackTrace.h
    Public/Core/Cli/CommandLine.h

    Public/Core/Compression/Compression.h

    Public/Core/Containers/ByteBuffer.h
    Public/Core/Containers/ConcurrentQueue.h
    Public/Core/Containers/Pow2Array.h
    Public/Core/Containers/SegmentedVector.h

    Public/Core/DI/Activator.h
    Public/Core/DI/BaseDI.h
    Public/Core/DI/Builder.h
    Public/Core/DI/Container.h
    Public/Core/DI/LifetimeScope.h
    Public/Core/DI/Registration.h
    Public/Core/DI/Registry.h

    Public/Core/IO/IAsyncStreamIO.h
    Public/Core/IO/BaseIO.h
    Public/Core/IO/FileStream.h
    Public/Core/IO/IStream.h
    Public/Core/IO/IStreamFactory.h
    Public/Core/IO/Path.h
    Public/Core/IO/PathParser.h
    Public/Core/IO/StreamBase.h

    Public/Core/Jobs/Base.h
    Public/Core/Jobs/IJobSystem.h
    Public/Core/Jobs/Job.h
    Public/Core/Jobs/JobSystem.h
    Public/Core/Jobs/TaskGraph.h
    Public/Core/Jobs/WaitGroup.h

    Public/Core/Logging/Logger.h
    Public/Core/Logging/Trace.h

    Public/Core/Math/Aabb.h
    Public/Core/Math/Color.h
    Public/Core/Math/Colors.h
    Public/Core/Math/Matrix4x4.h
    Public/Core/Math/Obb.h
    Public/Core/Math/Packing.h
    Public/Core/Math/Quaternion.h
    Public/Core/Math/Random.h
    Public/Core/Math/Rect.h
    Public/Core/Math/Sphere.h
    Public/Core/Math/Transform.h
    Public/Core/Math/UUID.h
    Public/Core/Math/Vector2.h
    Public/Core/Math/Vector3.h
    Public/Core/Math/Vector3Int.h
    Public/Core/Math/Vector3UInt.h
    Public/Core/Math/Vector4.h

    Public/Core/Memory/BuddyAllocator.h
    Public/Core/Memory/FiberTempAllocator.h
    Public/Core/Memory/LinearAllocator.h
    Public/Core/Memory/Memory.h
    Public/Core/Memory/PoolAllocator.h
    Public/Core/Memory/RingBufferAllocator.h
    Public/Core/Memory/RefCount.h
    Public/Core/Memory/SegmentedBuffer.h

    Public/Core/Modules/Configuration.h
    Public/Core/Modules/Environment.h
    Public/Core/Modules/LibraryLoader.h

    Public/Core/RTTI/Reflection.h
    Public/Core/RTTI/ReflectionContext.h
    Public/Core/RTTI/RTTI.h

    Public/Core/SIMD/Intersection.h
    Public/Core/SIMD/Soa.h
    Public/Core/SIMD/Utils.h

    Public/Core/Strings/Encoding.h
    Public/Core/Strings/Format.h
    Public/Core/Strings/Parser.h
    Public/Core/Strings/Utils.h

    Public/Core/Threading/Context.h
    Public/Core/Threading/ConditionVariable.h
    Public/Core/Threading/Event.h
    Public/Core/Threading/Fiber.h
    Public/Core/Threading/Mutex.h
    Public/Core/Threading/Semaphore.h
    Public/Core/Threading/SharedSpinLock.h
    Public/Core/Threading/SpinLock.h
    Public/Core/Threading/Thread.h

    Public/Core/Time/BaseTime.h
    Public/Core/Time/DateTime.h
    Public/Core/Time/TimeSpan.h

    Public/Core/Utils/Core.natvis
    Public/Core/Utils/Crc32.h
)

set(COMMON_SOURCES
    Private/Core/Base/Assert.cpp
    Private/Core/Cli/CommandLine.cpp
    Private/Core/Base/AssertPrivate.h
    Private/Core/Base/StackTrace.cpp
    Private/Core/Base/StackTracePrivate.h

    Private/Core/Compression/Compression.cpp
    Private/Core/Compression/CompressionPrivate.h

    Private/Core/DI/BaseDI.cpp
    Private/Core/DI/Builder.cpp
    Private/Core/DI/Container.cpp
    Private/Core/DI/LifetimeScope.cpp

    Private/Core/IO/Platform/PlatformFile.h
    Private/Core/IO/Platform/PlatformPath.h
    Private/Core/IO/AsyncStreamIO.h
    Private/Core/IO/AsyncStreamIO.cpp
    Private/Core/IO/DefaultAsyncIOBackend.cpp
    Private/Core/IO/BaseIO.cpp
    Private/Core/IO/BaseIOPrivate.h
    Private/Core/IO/FileStream.cpp
    Private/Core/IO/Path.cpp
    Private/Core/IO/StreamFactory.h
    Private/Core/IO/StreamFactory.cpp

    Private/Core/Jobs/Job.cpp
    Private/Core/Jobs/JobSystem.cpp
    Private/Core/Jobs/TaskGraph.cpp
    Private/Core/Jobs/WaitGroup.cpp

    Private/Core/Logging/Logger.cpp

    Private/Core/Math/Colors.cpp
    Private/Core/Math/Matrix4x4.cpp

    Private/Core/Memory/FiberTempAllocator.cpp
    Private/Core/Memory/LinearAllocator.cpp
    Private/Core/Memory/Memory.cpp
    Private/Core/Memory/MemoryPrivate.h
    Private/Core/Memory/PoolAllocator.cpp
    Private/Core/Memory/BuddyAllocator.cpp
    Private/Core/Memory/RingBufferAllocator.cpp
    Private/Core/Memory/tlsf.c
    Private/Core/Memory/tlsf.h

    Private/Core/Modules/Configuration.cpp
    Private/Core/Modules/Environment.cpp
    Private/Core/Modules/LibraryLoader.cpp

    Private/Core/Platform/Windows/Common.h
    Private/Core/Platform/Windows/Platform.cpp

    Private/Core/RTTI/Reflection.cpp
    Private/Core/RTTI/ReflectionPrivate.h

    Private/Core/Strings/Encoding.cpp
    Private/Core/Strings/Format.cpp
    Private/Core/Strings/Parser.cpp

    Private/Core/Threading/Context.cpp
    Private/Core/Threading/Fiber.cpp
    Private/Core/Threading/ThreadingPrivate.h

    Private/Core/Time/Platform/PlatformTime.cpp
    Private/Core/Time/DateTime.cpp

    Private/Core/Utils/Crc32.cpp
)

set(WINDOWS_SOURCES
    Private/Core/Base/Platform/Windows/PlatformAssert.cpp

    Private/Core/IO/Platform/Windows/PlatformFile.cpp
    Private/Core/IO/Platform/Windows/PlatformPath.cpp
    Private/Core/IO/Platform/Windows/OverlappedAsyncIOBackend.cpp

    Private/Core/Threading/Platform/Windows/Event.cpp
    Private/Core/Threading/Platform/Windows/ConditionVariable.cpp
    Private/Core/Threading/Platform/Windows/Thread.cpp
    Private/Core/Threading/Platform/Windows/Mutex.cpp
    Private/Core/Threading/Platform/Windows/Semaphore.cpp

    Private/Core/Time/Platform/Windows/PlatformTime.cpp
)

set(GENERATED_SOURCES
    Reflection.gen.cpp
)


add_library(FeCore STATIC ${PUBLIC_HEADERS} ${COMMON_SOURCES} ${WINDOWS_SOURCES} ${GENERATED_SOURCES})

fe_configure_target(FeCore)

target_include_directories(FeCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FeCore PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")

set_target_properties(FeCore PROPERTIES FOLDER "Core")
target_link_libraries(FeCore
    xxhash
    DebugHeap
    fcontext
    mimalloc-static
    libdeflate
    libzstd_static
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
