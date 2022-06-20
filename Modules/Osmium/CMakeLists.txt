set(SRC
    # FERRUM_SORT_LINES BEGIN
    GPU/OsmiumGPU.h

    GPU/Adapter/IAdapter.h
    GPU/Adapter/VKAdapter.cpp
    GPU/Adapter/VKAdapter.h

    GPU/Buffer/IBuffer.h
    GPU/Buffer/VKBuffer.cpp
    GPU/Buffer/VKBuffer.h

    GPU/CommandBuffer/ICommandBuffer.h
    GPU/CommandBuffer/VKCommandBuffer.cpp
    GPU/CommandBuffer/VKCommandBuffer.h

    GPU/CommandQueue/ICommandQueue.h
    GPU/CommandQueue/VKCommandQueue.cpp
    GPU/CommandQueue/VKCommandQueue.h

    GPU/Common/BaseTypes.h
    GPU/Common/VKConfig.cpp
    GPU/Common/VKConfig.h
    GPU/Common/VKViewport.h
    GPU/Common/Viewport.h

    GPU/Descriptors/DescriptorDesc.h
    GPU/Descriptors/IDescriptorHeap.h
    GPU/Descriptors/IDescriptorTable.h
    GPU/Descriptors/VKDescriptorHeap.cpp
    GPU/Descriptors/VKDescriptorHeap.h
    GPU/Descriptors/VKDescriptorTable.cpp
    GPU/Descriptors/VKDescriptorTable.h

    GPU/Device/IDevice.h
    GPU/Device/VKDevice.cpp
    GPU/Device/VKDevice.h

    GPU/Fence/FenceState.h
    GPU/Fence/IFence.h
    GPU/Fence/VKFence.cpp
    GPU/Fence/VKFence.h

    GPU/Framebuffer/IFramebuffer.h
    GPU/Framebuffer/VKFramebuffer.cpp
    GPU/Framebuffer/VKFramebuffer.h

    GPU/Image/IImage.h
    GPU/Image/ImageEnums.h
    GPU/Image/ImageFormat.h
    GPU/Image/ImageSubresource.h
    GPU/Image/VKImage.cpp
    GPU/Image/VKImage.h
    GPU/Image/VKImageFormat.h
    GPU/Image/VKImageSubresource.h

    GPU/ImageView/IImageView.h
    GPU/ImageView/VKImageView.cpp
    GPU/ImageView/VKImageView.h

    GPU/Instance/IInstance.cpp
    GPU/Instance/IInstance.h
    GPU/Instance/VKInstance.cpp
    GPU/Instance/VKInstance.h

    GPU/Memory/IDeviceMemory.h
    GPU/Memory/MemoryType.h
    GPU/Memory/VKDeviceMemory.cpp
    GPU/Memory/VKDeviceMemory.h

    GPU/Pipeline/IGraphicsPipeline.h
    GPU/Pipeline/InputLayoutBuilder.h
    GPU/Pipeline/InputStreamLayout.h
    GPU/Pipeline/PipelineStates.h
    GPU/Pipeline/VKGraphicsPipeline.cpp
    GPU/Pipeline/VKGraphicsPipeline.h

    GPU/RenderPass/IRenderPass.h
    GPU/RenderPass/VKRenderPass.cpp
    GPU/RenderPass/VKRenderPass.h

    GPU/Resource/BindFlags.h
    GPU/Resource/ResourceState.h
    GPU/Resource/ShaderResourceType.h
    GPU/Resource/VKResourceState.h

    GPU/Shader/IShaderCompiler.h
    GPU/Shader/IShaderModule.h
    GPU/Shader/IShaderReflection.h
    GPU/Shader/ShaderCompilerDXC.cpp
    GPU/Shader/ShaderCompilerDXC.h
    GPU/Shader/ShaderStage.h
    GPU/Shader/VKShaderModule.cpp
    GPU/Shader/VKShaderModule.h
    GPU/Shader/VKShaderReflection.cpp
    GPU/Shader/VKShaderReflection.h

    GPU/SwapChain/ISwapChain.h
    GPU/SwapChain/VKSwapChain.cpp
    GPU/SwapChain/VKSwapChain.h

    GPU/Window/IWindow.h
    GPU/Window/Window.cpp
    GPU/Window/Window.h
    # FERRUM_SORT_LINES END
)

add_library(OsmiumGPU SHARED ${SRC})

fe_configure_target(OsmiumGPU)

target_include_directories(OsmiumGPU PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")

set_target_properties(OsmiumGPU PROPERTIES FOLDER "Modules/Osmium")
target_link_libraries(OsmiumGPU vulkan glfw dxc FeCore spirv-cross-core spirv-cross-hlsl)

get_property("TARGET_SOURCE_FILES" TARGET OsmiumGPU PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

add_subdirectory(Bindings)
