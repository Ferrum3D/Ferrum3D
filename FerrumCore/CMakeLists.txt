file(GLOB_RECURSE PUBLIC_HEADERS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Public/*.h"
    "Public/*.natvis"
)

file(GLOB_RECURSE COMMON_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Private/*.c"
    "Private/*.cpp"
    "Private/*.h"
)

file(GLOB_RECURSE WINDOWS_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Private/Core/Platform/Windows/*.cpp"
    "Private/Core/Platform/Windows/*.h"
    "Private/Core/*/Platform/Windows/*.cpp"
    "Private/Core/*/Platform/Windows/*.h"
)
list(REMOVE_ITEM COMMON_SOURCES ${WINDOWS_SOURCES})

file(GLOB GENERATED_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.gen.cpp"
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
    dragonbox::dragonbox_to_chars
    rapidjson)

get_property("TARGET_SOURCE_FILES" TARGET FeCore PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

add_subdirectory(Tests)
