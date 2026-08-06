file(GLOB_RECURSE PUBLIC_HEADERS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Public/*.h"
)

file(GLOB COMMON_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Private/Graphics/Core/*.cpp"
    "Private/Graphics/Core/*.h"
)
file(GLOB_RECURSE COMMON_IMPLEMENTATION_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Private/Graphics/Core/Common/*.cpp"
    "Private/Graphics/Core/Common/*.h"
)
list(APPEND COMMON_SOURCES ${COMMON_IMPLEMENTATION_SOURCES})

file(GLOB_RECURSE VULKAN_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Private/Graphics/Core/Vulkan/*.cpp"
    "Private/Graphics/Core/Vulkan/*.h"
)

file(GLOB_RECURSE VULKAN_WINDOWS_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Private/Graphics/Core/Vulkan/Platform/Windows/*.cpp"
    "Private/Graphics/Core/Vulkan/Platform/Windows/*.h"
)
list(REMOVE_ITEM VULKAN_SOURCES ${VULKAN_WINDOWS_SOURCES})

file(GLOB GENERATED_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.gen.cpp"
)

add_library(FeGraphicsCore STATIC
    ${PUBLIC_HEADERS} ${COMMON_SOURCES} ${VULKAN_SOURCES} ${VULKAN_WINDOWS_SOURCES} ${GENERATED_SOURCES})

fe_configure_target(FeGraphicsCore)

target_include_directories(FeGraphicsCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FeGraphicsCore PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")

set_target_properties(FeGraphicsCore PROPERTIES FOLDER "Modules/Graphics")
target_link_libraries(FeGraphicsCore
    volk
    dxc
    dxil
    FeCore
    spirv-cross-core
    spirv-cross-hlsl
    GPUOpen::VulkanMemoryAllocator
    TracyClient
)

get_property("TARGET_SOURCE_FILES" TARGET FeGraphicsCore PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

if (EXISTS "${FE_THIRD_PARTY_DIR}/dxc/bin/x64/dxil.dll")
    add_custom_command(TARGET FeGraphicsCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${FE_THIRD_PARTY_DIR}/dxc/bin/x64/dxil.dll"
            $<TARGET_FILE_DIR:FeGraphicsCore>
    )
endif ()

if (EXISTS "${FE_THIRD_PARTY_DIR}/dxc/bin/x64/dxcompiler.dll")
    add_custom_command(TARGET FeGraphicsCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${FE_THIRD_PARTY_DIR}/dxc/bin/x64/dxcompiler.dll"
            $<TARGET_FILE_DIR:FeGraphicsCore>
    )
endif ()
