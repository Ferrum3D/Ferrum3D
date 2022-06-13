set(SRC
    GPU/Adapter/IAdapter.h
    GPU/Adapter/VKAdapter.h
    GPU/Adapter/VKAdapter.cpp

    GPU/Buffer/IBuffer.h
    GPU/Buffer/VKBuffer.h
    GPU/Buffer/VKBuffer.cpp

    GPU/CommandBuffer/ICommandBuffer.h
    GPU/CommandBuffer/VKCommandBuffer.h
    GPU/CommandBuffer/VKCommandBuffer.cpp

    GPU/CommandQueue/ICommandQueue.h
    GPU/CommandQueue/VKCommandQueue.h
    GPU/CommandQueue/VKCommandQueue.cpp

    GPU/Common/BaseTypes.h
    GPU/Common/Viewport.h
    GPU/Common/VKConfig.h
    GPU/Common/VKConfig.cpp

    GPU/Descriptors/IDescriptorHeap.h
    GPU/Descriptors/IDescriptorTable.h
    GPU/Descriptors/VKDescriptorHeap.h
    GPU/Descriptors/VKDescriptorHeap.cpp
    GPU/Descriptors/VKDescriptorTable.h
    GPU/Descriptors/VKDescriptorTable.cpp

    GPU/Device/IDevice.h
    GPU/Device/VKDevice.h
    GPU/Device/VKDevice.cpp

    GPU/Fence/IFence.h
    GPU/Fence/VKFence.h
    GPU/Fence/VKFence.cpp

    GPU/Framebuffer/IFramebuffer.h
    GPU/Framebuffer/VKFramebuffer.h
    GPU/Framebuffer/VKFramebuffer.cpp

    GPU/Image/IImage.h
    GPU/Image/ImageEnums.h
    GPU/Image/ImageFormat.h
    GPU/Image/ImageSubresource.h
    GPU/Image/VKImage.h
    GPU/Image/VKImage.cpp
    GPU/Image/VKImageFormat.h

    GPU/ImageView/IImageView.h
    GPU/ImageView/VKImageView.h
    GPU/ImageView/VKImageView.cpp

    GPU/Instance/IInstance.h
    GPU/Instance/IInstance.cpp
    GPU/Instance/VKInstance.h
    GPU/Instance/VKInstance.cpp

    GPU/Memory/IDeviceMemory.h
    GPU/Memory/VKDeviceMemory.h
    GPU/Memory/VKDeviceMemory.cpp

    GPU/Pipeline/IGraphicsPipeline.h
    GPU/Pipeline/InputLayoutBuilder.h
    GPU/Pipeline/InputStreamLayout.h
    GPU/Pipeline/PipelineStates.h
    GPU/Pipeline/VKGraphicsPipeline.h
    GPU/Pipeline/VKGraphicsPipeline.cpp

    GPU/RenderPass/IRenderPass.h
    GPU/RenderPass/VKRenderPass.h
    GPU/RenderPass/VKRenderPass.cpp

    GPU/Resource/IResource.h
    GPU/Resource/ResourceState.h

    GPU/Shader/IShaderCompiler.h
    GPU/Shader/IShaderModule.h
    GPU/Shader/IShaderReflection.h
    GPU/Shader/ShaderCompilerDXC.h
    GPU/Shader/ShaderCompilerDXC.cpp
    GPU/Shader/ShaderStage.h
    GPU/Shader/VKShaderModule.h
    GPU/Shader/VKShaderModule.cpp
    GPU/Shader/VKShaderReflection.h
    GPU/Shader/VKShaderReflection.cpp

    GPU/SwapChain/ISwapChain.h
    GPU/SwapChain/VKSwapChain.h
    GPU/SwapChain/VKSwapChain.cpp

    GPU/Window/IWindow.h
    GPU/Window/Window.h
    GPU/Window/Window.cpp)

add_library(OsmiumGPU SHARED ${SRC})

fe_configure_target(OsmiumGPU)

target_include_directories(OsmiumGPU PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")

set_target_properties(OsmiumGPU PROPERTIES FOLDER "Modules/Osmium")
target_link_libraries(OsmiumGPU vulkan glfw dxc FeCore spirv-cross-core spirv-cross-hlsl)

get_property("TARGET_SOURCE_FILES" TARGET OsmiumGPU PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

add_subdirectory(Bindings)
