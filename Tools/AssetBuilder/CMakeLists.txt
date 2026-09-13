file(GLOB_RECURSE PUBLIC_HEADERS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Public/*.h"
)

file(GLOB_RECURSE PRIVATE_SOURCES
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

add_library(AssetBuilder STATIC ${PUBLIC_HEADERS} ${PRIVATE_SOURCES} ${GENERATED_SOURCES})

fe_configure_target(AssetBuilder)

target_include_directories(AssetBuilder PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(AssetBuilder PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")

if(FE_COMPILER_MSVC)
    target_compile_options(AssetBuilder PRIVATE /wd5054)
endif()

set_target_properties(AssetBuilder PROPERTIES FOLDER "Tools")
target_link_libraries(AssetBuilder
    PUBLIC
        FeCore
        FeGraphicsCore
    PRIVATE
        meshoptimizer
        tiny_gltf
        rapidjson
        bcrypt)

get_property("TARGET_SOURCE_FILES" TARGET AssetBuilder PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
