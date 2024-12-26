set(PUBLIC_HEADERS
    Public/Graphics/RHI/Common/BaseTypes.h
    Public/Graphics/RHI/Common/Viewport.h

    Public/Graphics/RHI/FrameGraph/Base.h
    Public/Graphics/RHI/FrameGraph/Blackboard.h

    Public/Graphics/RHI/AdapterInfo.h
    Public/Graphics/RHI/Buffer.h
    Public/Graphics/RHI/CommandList.h
    Public/Graphics/RHI/CommandQueue.h
    Public/Graphics/RHI/Device.h
    Public/Graphics/RHI/DeviceObject.h
    Public/Graphics/RHI/DeviceService.h
    Public/Graphics/RHI/Fence.h
    Public/Graphics/RHI/Framebuffer.h
    Public/Graphics/RHI/Image.h
    Public/Graphics/RHI/ImageBase.h
    Public/Graphics/RHI/ImageFormat.h
    Public/Graphics/RHI/ImageSubresource.h
    Public/Graphics/RHI/ImageView.h
    Public/Graphics/RHI/DeviceFactory.h
    Public/Graphics/RHI/DeviceMemorySlice.h
    Public/Graphics/RHI/DeviceMemory.h
    Public/Graphics/RHI/GraphicsPipeline.h
    Public/Graphics/RHI/InputLayoutBuilder.h
    Public/Graphics/RHI/InputStreamLayout.h
    Public/Graphics/RHI/PipelineStates.h
    Public/Graphics/RHI/RenderPass.h
    Public/Graphics/RHI/Resource.h
    Public/Graphics/RHI/ResourceCache.h
    Public/Graphics/RHI/ResourcePool.h
    Public/Graphics/RHI/ResourceState.h
    Public/Graphics/RHI/ShaderResourceGroup.h
    Public/Graphics/RHI/ShaderResourceGroupData.h
    Public/Graphics/RHI/ShaderResourceType.h
    Public/Graphics/RHI/Sampler.h
    Public/Graphics/RHI/ShaderCompiler.h
    Public/Graphics/RHI/ShaderModule.h
    Public/Graphics/RHI/ShaderReflection.h
    Public/Graphics/RHI/ShaderCompilerDXC.h
    Public/Graphics/RHI/ShaderStage.h
    Public/Graphics/RHI/Swapchain.h
    Public/Graphics/RHI/IWindow.h
    Public/Graphics/RHI/Window.h
)

set(COMMON_SOURCES
    Private/Graphics/RHI/Device.cpp
    Private/Graphics/RHI/Module.cpp
    Private/Graphics/RHI/ShaderCompilerDXC.cpp
    Private/Graphics/RHI/Window.cpp
)

set(VULKAN_SOURCES
    Private/Graphics/RHI/Vulkan/Common/BaseTypes.h
    Private/Graphics/RHI/Vulkan/Common/Config.cpp
    Private/Graphics/RHI/Vulkan/Common/Config.h
    Private/Graphics/RHI/Vulkan/Common/Viewport.h

    Private/Graphics/RHI/Vulkan/Buffer.cpp
    Private/Graphics/RHI/Vulkan/Buffer.h
    Private/Graphics/RHI/Vulkan/CommandList.cpp
    Private/Graphics/RHI/Vulkan/CommandList.h
    Private/Graphics/RHI/Vulkan/CommandQueue.cpp
    Private/Graphics/RHI/Vulkan/CommandQueue.h
    Private/Graphics/RHI/Vulkan/DescriptorAllocator.cpp
    Private/Graphics/RHI/Vulkan/DescriptorAllocator.h
    Private/Graphics/RHI/Vulkan/Device.cpp
    Private/Graphics/RHI/Vulkan/Device.h
    Private/Graphics/RHI/Vulkan/DeviceFactory.cpp
    Private/Graphics/RHI/Vulkan/DeviceFactory.h
    Private/Graphics/RHI/Vulkan/DeviceMemory.cpp
    Private/Graphics/RHI/Vulkan/DeviceMemory.h
    Private/Graphics/RHI/Vulkan/Fence.cpp
    Private/Graphics/RHI/Vulkan/Fence.h
    Private/Graphics/RHI/Vulkan/Framebuffer.cpp
    Private/Graphics/RHI/Vulkan/Framebuffer.h
    Private/Graphics/RHI/Vulkan/Image.cpp
    Private/Graphics/RHI/Vulkan/Image.h
    Private/Graphics/RHI/Vulkan/ImageFormat.h
    Private/Graphics/RHI/Vulkan/ImageSubresource.h
    Private/Graphics/RHI/Vulkan/ImageView.cpp
    Private/Graphics/RHI/Vulkan/ImageView.h
    Private/Graphics/RHI/Vulkan/GraphicsPipeline.cpp
    Private/Graphics/RHI/Vulkan/GraphicsPipeline.h
    Private/Graphics/RHI/Vulkan/PipelineStates.h
    Private/Graphics/RHI/Vulkan/RenderPass.cpp
    Private/Graphics/RHI/Vulkan/RenderPass.h
    Private/Graphics/RHI/Vulkan/ResourcePool.h
    Private/Graphics/RHI/Vulkan/ResourcePool.cpp
    Private/Graphics/RHI/Vulkan/ResourceState.h
    Private/Graphics/RHI/Vulkan/Sampler.cpp
    Private/Graphics/RHI/Vulkan/Sampler.h
    Private/Graphics/RHI/Vulkan/ShaderModule.cpp
    Private/Graphics/RHI/Vulkan/ShaderModule.h
    Private/Graphics/RHI/Vulkan/ShaderReflection.cpp
    Private/Graphics/RHI/Vulkan/ShaderReflection.h
    Private/Graphics/RHI/Vulkan/ShaderResourceGroup.cpp
    Private/Graphics/RHI/Vulkan/ShaderResourceGroup.h
    Private/Graphics/RHI/Vulkan/Swapchain.cpp
    Private/Graphics/RHI/Vulkan/Swapchain.h
    Private/Graphics/RHI/Vulkan/VmaImpl.cpp
)


add_library(FeGraphicsRHI SHARED ${PUBLIC_HEADERS} ${VULKAN_SOURCES} ${COMMON_SOURCES})

fe_configure_target(FeGraphicsRHI)

target_include_directories(FeGraphicsRHI PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FeGraphicsRHI PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")

set_target_properties(FeGraphicsRHI PROPERTIES FOLDER "Modules/Graphics")
target_link_libraries(FeGraphicsRHI
    volk
    glfw
    dxc
    FeCore
    spirv-cross-core
    spirv-cross-hlsl
    GPUOpen::VulkanMemoryAllocator
    TracyClient
)

get_property("TARGET_SOURCE_FILES" TARGET FeGraphicsRHI PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
