set(PUBLIC_HEADERS
    Public/Graphics/Core/Base/BaseTypes.h
    Public/Graphics/Core/Base/Limits.h

    Public/Graphics/Core/FrameGraph/Base.h
    Public/Graphics/Core/FrameGraph/Blackboard.h
    Public/Graphics/Core/FrameGraph/FrameGraph.h
    Public/Graphics/Core/FrameGraph/FrameGraphContext.h

    Public/Graphics/Core/AdapterInfo.h
    Public/Graphics/Core/AsyncCopyQueue.h
    Public/Graphics/Core/Buffer.h
    Public/Graphics/Core/Device.h
    Public/Graphics/Core/DeviceFactory.h
    Public/Graphics/Core/DeviceObject.h
    Public/Graphics/Core/DrawArguments.h
    Public/Graphics/Core/DrawListBuilder.h
    Public/Graphics/Core/Fence.h
    Public/Graphics/Core/GeometryPool.h
    Public/Graphics/Core/GeometryView.h
    Public/Graphics/Core/GraphicsPipeline.h
    Public/Graphics/Core/Image.h
    Public/Graphics/Core/ImageBase.h
    Public/Graphics/Core/ImageFormat.h
    Public/Graphics/Core/InputLayoutBuilder.h
    Public/Graphics/Core/InputStreamLayout.h
    Public/Graphics/Core/Module.h
    Public/Graphics/Core/PipelineFactory.h
    Public/Graphics/Core/PipelineStates.h
    Public/Graphics/Core/Resource.h
    Public/Graphics/Core/ResourcePool.h
    Public/Graphics/Core/ResourceState.h
    Public/Graphics/Core/Sampler.h
    Public/Graphics/Core/ShaderCompiler.h
    Public/Graphics/Core/ShaderDefines.h
    Public/Graphics/Core/ShaderLibrary.h
    Public/Graphics/Core/ShaderReflection.h
    Public/Graphics/Core/ShaderResourceType.h
    Public/Graphics/Core/ShaderStage.h
    Public/Graphics/Core/Viewport.h
)

set(COMMON_SOURCES
    Private/Graphics/Core/Common/Device.cpp
    Private/Graphics/Core/Common/Device.h
    Private/Graphics/Core/Common/ShaderSourceCache.cpp
    Private/Graphics/Core/Common/ShaderSourceCache.h

    Private/Graphics/Core/Common/FrameGraph/FrameGraph.cpp
    Private/Graphics/Core/Common/FrameGraph/FrameGraph.h
    Private/Graphics/Core/Common/FrameGraph/FrameGraphContext.cpp
    Private/Graphics/Core/Common/FrameGraph/FrameGraphContext.h
    Private/Graphics/Core/Common/FrameGraph/FrameGraphResourcePool.cpp
    Private/Graphics/Core/Common/FrameGraph/FrameGraphResourcePool.h

    Private/Graphics/Core/Module.cpp
    Private/Graphics/Core/ShaderCompilerDXC.cpp
    Private/Graphics/Core/ShaderCompilerDXC.h
)

set(VULKAN_SOURCES
    Private/Graphics/Core/Vulkan/VmaImpl.cpp

    Private/Graphics/Core/Vulkan/Base/BaseTypes.cpp
    Private/Graphics/Core/Vulkan/Base/BaseTypes.h
    Private/Graphics/Core/Vulkan/Base/Config.cpp
    Private/Graphics/Core/Vulkan/Base/Config.h
    Private/Graphics/Core/Vulkan/Base/Viewport.h

    Private/Graphics/Core/Vulkan/FrameGraph/FrameGraph.cpp
    Private/Graphics/Core/Vulkan/FrameGraph/FrameGraph.h
    Private/Graphics/Core/Vulkan/FrameGraph/FrameGraphContext.cpp
    Private/Graphics/Core/Vulkan/FrameGraph/FrameGraphContext.h

    Private/Graphics/Core/Vulkan/Platform/VulkanSurface.h

    Private/Graphics/Core/Vulkan/AsyncCopyQueue.cpp
    Private/Graphics/Core/Vulkan/AsyncCopyQueue.h
    Private/Graphics/Core/Vulkan/Buffer.cpp
    Private/Graphics/Core/Vulkan/Buffer.h
    Private/Graphics/Core/Vulkan/DescriptorAllocator.cpp
    Private/Graphics/Core/Vulkan/DescriptorAllocator.h
    Private/Graphics/Core/Vulkan/Device.cpp
    Private/Graphics/Core/Vulkan/Device.h
    Private/Graphics/Core/Vulkan/DeviceFactory.cpp
    Private/Graphics/Core/Vulkan/DeviceFactory.h
    Private/Graphics/Core/Vulkan/Fence.cpp
    Private/Graphics/Core/Vulkan/Fence.h
    Private/Graphics/Core/Vulkan/GeometryPool.cpp
    Private/Graphics/Core/Vulkan/GeometryPool.h
    Private/Graphics/Core/Vulkan/GraphicsPipeline.cpp
    Private/Graphics/Core/Vulkan/GraphicsPipeline.h
    Private/Graphics/Core/Vulkan/Image.cpp
    Private/Graphics/Core/Vulkan/Image.h
    Private/Graphics/Core/Vulkan/ImageFormat.h
    Private/Graphics/Core/Vulkan/PipelineFactory.cpp
    Private/Graphics/Core/Vulkan/PipelineFactory.h
    Private/Graphics/Core/Vulkan/PipelineStates.h
    Private/Graphics/Core/Vulkan/ResourceBarrierBatcher.cpp
    Private/Graphics/Core/Vulkan/ResourceBarrierBatcher.h
    Private/Graphics/Core/Vulkan/ResourcePool.cpp
    Private/Graphics/Core/Vulkan/ResourcePool.h
    Private/Graphics/Core/Vulkan/ResourceState.h
    Private/Graphics/Core/Vulkan/Sampler.cpp
    Private/Graphics/Core/Vulkan/Sampler.h
    Private/Graphics/Core/Vulkan/ShaderLibrary.cpp
    Private/Graphics/Core/Vulkan/ShaderLibrary.h
    Private/Graphics/Core/Vulkan/ShaderReflection.cpp
    Private/Graphics/Core/Vulkan/ShaderReflection.h
    Private/Graphics/Core/Vulkan/Viewport.cpp
    Private/Graphics/Core/Vulkan/Viewport.h
)


set(VULKAN_WINDOWS_SOURCES
    Private/Graphics/Core/Vulkan/Platform/Windows/VulkanSurface.cpp
)


add_library(FeGraphicsCore SHARED ${PUBLIC_HEADERS} ${COMMON_SOURCES} ${VULKAN_SOURCES} ${VULKAN_WINDOWS_SOURCES})

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

add_custom_command(TARGET FeGraphicsCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FE_THIRD_PARTY_DIR}/dxc/bin/x64/dxil.dll"
        $<TARGET_FILE_DIR:FeGraphicsCore>
)

add_custom_command(TARGET FeGraphicsCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FE_THIRD_PARTY_DIR}/dxc/bin/x64/dxcompiler.dll"
        $<TARGET_FILE_DIR:FeGraphicsCore>
)
