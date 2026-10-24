file(GLOB_RECURSE SHADER_SHARED_HEADERS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Shaders/Shaders/*.h"
)

file(GLOB_RECURSE SHADER_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Shaders/Shaders/*.hlsl"
    "Shaders/Shaders/*.hlsli"
)
set_source_files_properties(${SHADER_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

file(GLOB_RECURSE PUBLIC_HEADERS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Public/*.h"
)

file(GLOB_RECURSE COMMON_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Private/*.cpp"
    "Private/*.h"
)

file(GLOB GENERATED_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.gen.cpp"
)

add_library(FeGraphics STATIC
    ${PUBLIC_HEADERS} ${COMMON_SOURCES} ${SHADER_SHARED_HEADERS} ${SHADER_SOURCES} ${GENERATED_SOURCES})

fe_configure_target(FeGraphics)

target_include_directories(FeGraphics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FeGraphics PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")
target_include_directories(FeGraphics PRIVATE "${FE_THIRD_PARTY_DIR}/hosek")
set_source_files_properties(Private/Graphics/Features/Lighting/HosekSkyModel.cpp
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
target_include_directories(FeGraphics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
target_include_directories(FeGraphics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ThirdParty/FidelityFX")
target_compile_definitions(FeGraphics PRIVATE FE_GRAPHICS_SHADER_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/Shaders")

set_target_properties(FeGraphics PROPERTIES FOLDER "Modules/Graphics")
target_link_libraries(FeGraphics FeGraphicsCore TracyClient imgui)

get_property("TARGET_SOURCE_FILES" TARGET FeGraphics PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

add_subdirectory(Tests)
