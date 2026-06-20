set(PUBLIC_HEADERS
    Public/AssetBuilder/ModelProcessor.h
    Public/AssetBuilder/TextureProcessor.h
)

set(PRIVATE_SOURCES
    Private/AssetBuilder/Utils.cpp
    Private/AssetBuilder/Utils.h
    Private/AssetBuilder/TextureProcessor.cpp
    Private/AssetBuilder/MeshOptimization.cpp
    Private/AssetBuilder/MeshOptimization.h
    Private/AssetBuilder/ModelImporter.cpp
    Private/AssetBuilder/ModelImporter.h
    Private/AssetBuilder/ModelProcessor.cpp
)

add_library(AssetBuilder STATIC ${PUBLIC_HEADERS} ${PRIVATE_SOURCES})

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
        FeGraphics
        compressonator
        meshoptimizer
        stb
        tiny_gltf
        rapidjson)

get_property("TARGET_SOURCE_FILES" TARGET AssetBuilder PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})