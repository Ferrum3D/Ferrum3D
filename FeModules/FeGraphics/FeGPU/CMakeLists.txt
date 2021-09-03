set(SRC
    Adapter/IAdapter.h
    Adapter/VKAdapter.h
    Adapter/VKAdapter.cpp

    Buffer/IBuffer.h
    Buffer/VKBuffer.h
    Buffer/VKBuffer.cpp

    CommandBuffer/ICommandBuffer.h
    CommandBuffer/VKCommandBuffer.h
    CommandBuffer/VKCommandBuffer.cpp

    CommandQueue/ICommandQueue.h
    CommandQueue/VKCommandQueue.h
    CommandQueue/VKCommandQueue.cpp

    Common/BaseTypes.h
    Common/Viewport.h
    Common/VKConfig.h
    Common/VKConfig.cpp

    Descriptors/IDescriptorHeap.h
    Descriptors/IDescriptorTable.h
    Descriptors/VKDescriptorHeap.h
    Descriptors/VKDescriptorHeap.cpp
    Descriptors/VKDescriptorTable.h
    Descriptors/VKDescriptorTable.cpp

    Device/IDevice.h
    Device/VKDevice.h
    Device/VKDevice.cpp

    Fence/IFence.h
    Fence/VKFence.h
    Fence/VKFence.cpp

    Framebuffer/IFramebuffer.h
    Framebuffer/VKFramebuffer.h
    Framebuffer/VKFramebuffer.cpp

    Image/IImage.h
    Image/IImage.cpp
    Image/ImageEnums.h
    Image/ImageFormat.h
    Image/ImageFormat.cpp
    Image/ImageSubresource.h
    Image/VKImage.h
    Image/VKImage.cpp
    Image/VKImageFormat.h

    ImageView/IImageView.h
    ImageView/VKImageView.h
    ImageView/VKImageView.cpp

    Instance/IInstance.h
    Instance/IInstance.cpp
    Instance/VKInstance.h
    Instance/VKInstance.cpp

    Memory/IDeviceMemory.h
    Memory/VKDeviceMemory.h
    Memory/VKDeviceMemory.cpp

    Pipeline/IGraphicsPipeline.h
    Pipeline/InputLayoutBuilder.h
    Pipeline/InputStreamLayout.h
    Pipeline/PipelineStates.h
    Pipeline/VKGraphicsPipeline.h
    Pipeline/VKGraphicsPipeline.cpp

    RenderPass/IRenderPass.h
    RenderPass/VKRenderPass.h
    RenderPass/VKRenderPass.cpp

    Resource/IResource.h
    Resource/ResourceState.h

    Shader/IShaderCompiler.h
    Shader/IShaderModule.h
    Shader/IShaderReflection.h
    Shader/ShaderCompilerDXC.h
    Shader/ShaderCompilerDXC.cpp
    Shader/ShaderStage.h
    Shader/VKShaderModule.h
    Shader/VKShaderModule.cpp
    Shader/VKShaderReflection.h
    Shader/VKShaderReflection.cpp

    SwapChain/ISwapChain.h
    SwapChain/VKSwapChain.h
    SwapChain/VKSwapChain.cpp

    Window/IWindow.h
    Window/Window.h
    Window/Window.cpp)

add_library(FeGPU STATIC ${SRC})

fe_configure_target(FeGPU)

target_include_directories(FeGPU PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

set_target_properties(FeGPU PROPERTIES FOLDER "Modules/FeGraphics/FeGPU")
target_link_libraries(FeGPU vulkan glfw dxc FeCore spirv-cross-core spirv-cross-hlsl)

get_property("TARGET_SOURCE_FILES" TARGET FeGPU PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
