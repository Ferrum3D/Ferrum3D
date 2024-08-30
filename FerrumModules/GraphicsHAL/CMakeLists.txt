set(PUBLIC_HEADERS
    Public/HAL/Common/BaseTypes.h
    Public/HAL/Common/Viewport.h

    Public/HAL/AdapterInfo.h
    Public/HAL/Buffer.h
    Public/HAL/CommandList.h
    Public/HAL/CommandQueue.h
    Public/HAL/Device.h
    Public/HAL/DeviceObject.h
    Public/HAL/DeviceService.h
    Public/HAL/Fence.h
    Public/HAL/Framebuffer.h
    Public/HAL/Image.h
    Public/HAL/ImageBase.h
    Public/HAL/ImageEnums.h
    Public/HAL/ImageFormat.h
    Public/HAL/ImageSubresource.h
    Public/HAL/ImageView.h
    Public/HAL/DeviceFactory.h
    Public/HAL/DeviceMemorySlice.h
    Public/HAL/DeviceMemory.h
    Public/HAL/MemoryType.h
    Public/HAL/GraphicsPipeline.h
    Public/HAL/InputLayoutBuilder.h
    Public/HAL/InputStreamLayout.h
    Public/HAL/PipelineStates.h
    Public/HAL/RenderPass.h
    Public/HAL/BindFlags.h
    Public/HAL/TransientResourceHeap.h
    Public/HAL/Resource.h
    Public/HAL/ResourceCache.h
    Public/HAL/ResourceState.h
    Public/HAL/ShaderResourceGroup.h
    Public/HAL/ShaderResourceGroupData.h
    Public/HAL/ShaderResourceType.h
    Public/HAL/TransientResourceHeapBase.h
    Public/HAL/Sampler.h
    Public/HAL/ShaderCompiler.h
    Public/HAL/ShaderModule.h
    Public/HAL/ShaderReflection.h
    Public/HAL/ShaderCompilerDXC.h
    Public/HAL/ShaderStage.h
    Public/HAL/Swapchain.h
    Public/HAL/IWindow.h
    Public/HAL/Window.h
)

set(COMMON_SOURCES
    Source/Device.cpp
    Source/Module.cpp
    Source/ShaderCompilerDXC.cpp
    Source/TransientResourceHeapBase.cpp
    Source/Window.cpp
)

set(VULKAN_SOURCES
    Private/HAL/Vulkan/Common/BaseTypes.h
    Private/HAL/Vulkan/Common/Config.cpp
    Private/HAL/Vulkan/Common/Config.h
    Private/HAL/Vulkan/Common/Viewport.h

    Private/HAL/Vulkan/Buffer.cpp
    Private/HAL/Vulkan/Buffer.h
    Private/HAL/Vulkan/CommandList.cpp
    Private/HAL/Vulkan/CommandList.h
    Private/HAL/Vulkan/CommandQueue.cpp
    Private/HAL/Vulkan/CommandQueue.h
    Private/HAL/Vulkan/DescriptorAllocator.cpp
    Private/HAL/Vulkan/DescriptorAllocator.h
    Private/HAL/Vulkan/Device.cpp
    Private/HAL/Vulkan/Device.h
    Private/HAL/Vulkan/DeviceFactory.cpp
    Private/HAL/Vulkan/DeviceFactory.h
    Private/HAL/Vulkan/DeviceMemory.cpp
    Private/HAL/Vulkan/DeviceMemory.h
    Private/HAL/Vulkan/Fence.cpp
    Private/HAL/Vulkan/Fence.h
    Private/HAL/Vulkan/Framebuffer.cpp
    Private/HAL/Vulkan/Framebuffer.h
    Private/HAL/Vulkan/Image.cpp
    Private/HAL/Vulkan/Image.h
    Private/HAL/Vulkan/ImageFormat.h
    Private/HAL/Vulkan/ImageSubresource.h
    Private/HAL/Vulkan/ImageView.cpp
    Private/HAL/Vulkan/ImageView.h
    Private/HAL/Vulkan/MemoryRequirementsCache.cpp
    Private/HAL/Vulkan/MemoryRequirementsCache.h
    Private/HAL/Vulkan/GraphicsPipeline.cpp
    Private/HAL/Vulkan/GraphicsPipeline.h
    Private/HAL/Vulkan/PipelineStates.h
    Private/HAL/Vulkan/RenderPass.cpp
    Private/HAL/Vulkan/RenderPass.h
    Private/HAL/Vulkan/ResourceState.h
    Private/HAL/Vulkan/TransientResourceHeap.cpp
    Private/HAL/Vulkan/TransientResourceHeap.h
    Private/HAL/Vulkan/Sampler.cpp
    Private/HAL/Vulkan/Sampler.h
    Private/HAL/Vulkan/ShaderModule.cpp
    Private/HAL/Vulkan/ShaderModule.h
    Private/HAL/Vulkan/ShaderReflection.cpp
    Private/HAL/Vulkan/ShaderReflection.h
    Private/HAL/Vulkan/ShaderResourceGroup.cpp
    Private/HAL/Vulkan/ShaderResourceGroup.h
    Private/HAL/Vulkan/Swapchain.cpp
    Private/HAL/Vulkan/Swapchain.h
)


add_library(FeGraphicsHAL SHARED ${PUBLIC_HEADERS} ${VULKAN_SOURCES} ${COMMON_SOURCES})

fe_configure_target(FeGraphicsHAL)

target_include_directories(FeGraphicsHAL PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FeGraphicsHAL PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")

set_target_properties(FeGraphicsHAL PROPERTIES FOLDER "Modules/Graphics")
target_link_libraries(FeGraphicsHAL volk glfw dxc FeCore spirv-cross-core spirv-cross-hlsl TracyClient)

get_property("TARGET_SOURCE_FILES" TARGET FeGraphicsHAL PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
