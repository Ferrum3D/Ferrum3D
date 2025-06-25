set(SHADER_SHARED_HEADERS
    Shaders/Shaders/Base/Base.h

    Shaders/Shaders/Features/Tools/Blit/Blit.h
    Shaders/Shaders/Features/Tools/Downsample/Downsample.h
)

set(SHADER_SOURCES
    Shaders/Shaders/Base/Base.hlsli
    Shaders/Shaders/Base/BaseTypes.hlsli
    Shaders/Shaders/Base/Descriptors.hlsli

    Shaders/Shaders/Features/Tools/Blit/Blit.hlsli
    Shaders/Shaders/Features/Tools/Blit/Blit.ps.hlsl
    Shaders/Shaders/Features/Tools/Blit/Blit.vs.hlsl
    Shaders/Shaders/Features/Tools/Downsample/Downsample.cs.hlsl
)

set_source_files_properties(${SHADER_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

set(PUBLIC_HEADERS
    Public/Graphics/Assets/Base.h
    Public/Graphics/Assets/IModelAssetManager.h
    Public/Graphics/Assets/ITextureAssetManager.h
    Public/Graphics/Assets/ModelAssetFormat.h
    Public/Graphics/Assets/TextureAssetFormat.h

    Public/Graphics/Features/Tools/Blit.h
    Public/Graphics/Features/Tools/Downsample.h

    Public/Graphics/Module.h
)

set(COMMON_SOURCES
    Private/Graphics/Assets/ModelAssetManager.cpp
    Private/Graphics/Assets/ModelAssetManager.h
    Private/Graphics/Assets/TextureAssetManager.cpp
    Private/Graphics/Assets/TextureAssetManager.h

    Private/Graphics/Features/Tools/Blit.cpp
    Private/Graphics/Features/Tools/Downsample.cpp

    Private/Graphics/Module.cpp
)

add_library(FeGraphics STATIC ${PUBLIC_HEADERS} ${COMMON_SOURCES} ${SHADER_SHARED_HEADERS} ${SHADER_SOURCES})

fe_configure_target(FeGraphics)

target_include_directories(FeGraphics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FeGraphics PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")
target_include_directories(FeGraphics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
target_include_directories(FeGraphics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ThirdParty/FidelityFX")

set_target_properties(FeGraphics PROPERTIES FOLDER "Modules/Graphics")
target_link_libraries(FeGraphics FeGraphicsCore TracyClient)

get_property("TARGET_SOURCE_FILES" TARGET FeGraphics PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
