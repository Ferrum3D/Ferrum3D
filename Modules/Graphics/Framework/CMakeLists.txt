set(SHADER_SHARED_HEADERS
    Shaders/Shaders/Base/Base.h

    Shaders/Shaders/Database/Base.h

    Shaders/Shaders/Passes/Tools/Blit/Blit.h
    Shaders/Shaders/Passes/Tools/Downsample/Downsample.h
)

set(SHADER_SOURCES
    Shaders/Shaders/Base/Base.hlsli
    Shaders/Shaders/Base/BaseTypes.hlsli
    Shaders/Shaders/Base/Descriptors.hlsli

    Shaders/Shaders/Database/Base.hlsli

    Shaders/Shaders/Passes/Tools/Blit/Blit.hlsli
    Shaders/Shaders/Passes/Tools/Blit/Blit.ps.hlsl
    Shaders/Shaders/Passes/Tools/Blit/Blit.vs.hlsl
    Shaders/Shaders/Passes/Tools/Downsample/Downsample.cs.hlsl

    Shaders/Shaders/Platform/Vulkan/Intrinsics.hlsli
)

set_source_files_properties(${SHADER_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

set(PUBLIC_HEADERS
    Public/Graphics/Assets/Base.h
    Public/Graphics/Assets/IModelAssetManager.h
    Public/Graphics/Assets/ITextureAssetManager.h
    Public/Graphics/Assets/ModelAssetFormat.h
    Public/Graphics/Assets/TextureAssetFormat.h

    Public/Graphics/Base/Base.h
    Public/Graphics/Base/BaseModuleList.h
    Public/Graphics/Base/DrawTag.h

    Public/Graphics/Database/Base.h
    Public/Graphics/Database/Database.h

    Public/Graphics/Passes/Tools/Blit.h
    Public/Graphics/Passes/Tools/Downsample.h

    Public/Graphics/Scene/Scene.h
    Public/Graphics/Scene/View.h

    Public/Graphics/Module.h
    Public/Graphics/Renderer.h
)

set(COMMON_SOURCES
    Private/Graphics/Assets/ModelAssetManager.cpp
    Private/Graphics/Assets/ModelAssetManager.h
    Private/Graphics/Assets/TextureAssetManager.cpp
    Private/Graphics/Assets/TextureAssetManager.h

    Private/Graphics/Base/DrawTag.cpp

    Private/Graphics/Database/Database.cpp

    Private/Graphics/Passes/Tools/Blit.cpp
    Private/Graphics/Passes/Tools/Downsample.cpp

    Private/Graphics/Scene/SceneImpl.cpp
    Private/Graphics/Scene/SceneImpl.h
    Private/Graphics/Scene/View.cpp
    Private/Graphics/Scene/ViewImpl.cpp
    Private/Graphics/Scene/ViewImpl.h

    Private/Graphics/Module.cpp
    Private/Graphics/RendererImpl.cpp
    Private/Graphics/RendererImpl.h
)

set(GENERATED_SOURCES
    Reflection.gen.cpp
)


add_library(FeGraphics STATIC
    ${PUBLIC_HEADERS} ${COMMON_SOURCES} ${SHADER_SHARED_HEADERS} ${SHADER_SOURCES} ${GENERATED_SOURCES})

fe_configure_target(FeGraphics)

target_include_directories(FeGraphics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FeGraphics PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")
target_include_directories(FeGraphics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Shaders")
target_include_directories(FeGraphics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/ThirdParty/FidelityFX")

set_target_properties(FeGraphics PROPERTIES FOLDER "Modules/Graphics")
target_link_libraries(FeGraphics FeGraphicsCore TracyClient)

get_property("TARGET_SOURCE_FILES" TARGET FeGraphics PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
