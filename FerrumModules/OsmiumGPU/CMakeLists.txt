set(SRC
    # FERRUM_SORT_LINES BEGIN
    OsGPU/OsmiumGPU.h
    OsGPU/OsmiumGPUModule.cpp
    OsGPU/OsmiumGPUModule.h

    OsGPU/Adapter/IAdapter.h
    OsGPU/Adapter/VKAdapter.cpp
    OsGPU/Adapter/VKAdapter.h

    OsGPU/Buffer/IBuffer.h
    OsGPU/Buffer/VKBuffer.cpp
    OsGPU/Buffer/VKBuffer.h

    OsGPU/CommandBuffer/ICommandBuffer.h
    OsGPU/CommandBuffer/VKCommandBuffer.cpp
    OsGPU/CommandBuffer/VKCommandBuffer.h

    OsGPU/CommandQueue/ICommandQueue.h
    OsGPU/CommandQueue/VKCommandQueue.cpp
    OsGPU/CommandQueue/VKCommandQueue.h

    OsGPU/Common/BaseTypes.h
    OsGPU/Common/VKBaseTypes.h
    OsGPU/Common/VKConfig.cpp
    OsGPU/Common/VKConfig.h
    OsGPU/Common/VKViewport.h
    OsGPU/Common/Viewport.h

    OsGPU/Descriptors/DescriptorDesc.h
    OsGPU/Descriptors/IDescriptorHeap.h
    OsGPU/Descriptors/IDescriptorTable.h
    OsGPU/Descriptors/VKDescriptorHeap.cpp
    OsGPU/Descriptors/VKDescriptorHeap.h
    OsGPU/Descriptors/VKDescriptorTable.cpp
    OsGPU/Descriptors/VKDescriptorTable.h

    OsGPU/Device/IDevice.h
    OsGPU/Device/VKDevice.cpp
    OsGPU/Device/VKDevice.h

    OsGPU/Fence/FenceState.h
    OsGPU/Fence/IFence.h
    OsGPU/Fence/VKFence.cpp
    OsGPU/Fence/VKFence.h

    OsGPU/Framebuffer/IFramebuffer.h
    OsGPU/Framebuffer/VKFramebuffer.cpp
    OsGPU/Framebuffer/VKFramebuffer.h

    OsGPU/Image/IImage.h
    OsGPU/Image/ImageBase.h
    OsGPU/Image/ImageEnums.h
    OsGPU/Image/ImageFormat.h
    OsGPU/Image/ImageSubresource.h
    OsGPU/Image/VKImage.cpp
    OsGPU/Image/VKImage.h
    OsGPU/Image/VKImageFormat.h
    OsGPU/Image/VKImageSubresource.h

    OsGPU/ImageView/IImageView.h
    OsGPU/ImageView/VKImageView.cpp
    OsGPU/ImageView/VKImageView.h

    OsGPU/Instance/IInstance.cpp
    OsGPU/Instance/IInstance.h
    OsGPU/Instance/VKInstance.cpp
    OsGPU/Instance/VKInstance.h

    OsGPU/Memory/IDeviceMemory.h
    OsGPU/Memory/MemoryType.h
    OsGPU/Memory/VKDeviceMemory.cpp
    OsGPU/Memory/VKDeviceMemory.h

    OsGPU/Pipeline/IGraphicsPipeline.h
    OsGPU/Pipeline/InputLayoutBuilder.h
    OsGPU/Pipeline/InputStreamLayout.h
    OsGPU/Pipeline/PipelineStates.h
    OsGPU/Pipeline/VKGraphicsPipeline.cpp
    OsGPU/Pipeline/VKGraphicsPipeline.h
    OsGPU/Pipeline/VKPipelineStates.h

    OsGPU/RenderPass/IRenderPass.h
    OsGPU/RenderPass/VKRenderPass.cpp
    OsGPU/RenderPass/VKRenderPass.h

    OsGPU/Resource/BindFlags.h
    OsGPU/Resource/ResourceState.h
    OsGPU/Resource/ShaderResourceType.h
    OsGPU/Resource/VKResourceState.h

    OsGPU/Sampler/ISampler.h
    OsGPU/Sampler/VKSampler.cpp
    OsGPU/Sampler/VKSampler.h

    OsGPU/Shader/IShaderCompiler.h
    OsGPU/Shader/IShaderModule.h
    OsGPU/Shader/IShaderReflection.h
    OsGPU/Shader/ShaderCompilerDXC.cpp
    OsGPU/Shader/ShaderCompilerDXC.h
    OsGPU/Shader/ShaderStage.h
    OsGPU/Shader/VKShaderModule.cpp
    OsGPU/Shader/VKShaderModule.h
    OsGPU/Shader/VKShaderReflection.cpp
    OsGPU/Shader/VKShaderReflection.h

    OsGPU/SwapChain/ISwapChain.h
    OsGPU/SwapChain/VKSwapChain.cpp
    OsGPU/SwapChain/VKSwapChain.h

    OsGPU/Window/IWindow.h
    OsGPU/Window/Window.cpp
    OsGPU/Window/Window.h
    OsGPU/Memory/DeviceMemorySlice.h
    # FERRUM_SORT_LINES END
    )

add_library(OsGPU SHARED ${SRC})

fe_configure_target(OsGPU)

target_include_directories(OsGPU PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")

set_target_properties(OsGPU PROPERTIES FOLDER "Modules/Osmium")
target_link_libraries(OsGPU volk glfw dxc FeCore spirv-cross-core spirv-cross-hlsl)

get_property("TARGET_SOURCE_FILES" TARGET OsGPU PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

add_subdirectory(Bindings)
