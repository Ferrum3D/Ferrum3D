set(PUBLIC_HEADERS
    Public/Graphics/Assets/ImageAssetLoader.h
    Public/Graphics/Assets/ImageAssetStorage.h
    Public/Graphics/Assets/MeshAssetLoader.h
    Public/Graphics/Assets/MeshAssetStorage.h
    Public/Graphics/Assets/ShaderAssetLoader.h
    Public/Graphics/Assets/ShaderAssetStorage.h

    Public/Graphics/Module.h
)

set(COMMON_SOURCES
    Private/Graphics/Assets/ImageAssetLoader.cpp
    Private/Graphics/Assets/ImageAssetStorage.cpp
    Private/Graphics/Assets/ImageLoaderImpl.h
    Private/Graphics/Assets/ImageLoaderImpl.cpp
    Private/Graphics/Assets/MeshAssetLoader.cpp
    Private/Graphics/Assets/MeshAssetStorage.cpp
    Private/Graphics/Assets/MeshLoaderImpl.h
    Private/Graphics/Assets/MeshLoaderImpl.cpp
    Private/Graphics/Assets/ShaderAssetLoader.cpp
    Private/Graphics/Assets/ShaderAssetStorage.cpp

    Private/Graphics/Module.cpp
)

add_library(FeGraphics SHARED ${PUBLIC_HEADERS} ${COMMON_SOURCES})

fe_configure_target(FeGraphics)

target_include_directories(FeGraphics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FeGraphics PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")

set_target_properties(FeGraphics PROPERTIES FOLDER "Modules/Graphics")
target_link_libraries(FeGraphics FeGraphicsHAL bc7enc stb assimp TracyClient)

get_property("TARGET_SOURCE_FILES" TARGET FeGraphics PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
