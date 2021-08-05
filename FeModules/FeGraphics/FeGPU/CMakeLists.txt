set(SRC
    Adapter/IAdapter.h
    Adapter/VKAdapter.h
    Adapter/VKAdapter.cpp

    CommandQueue/ICommandQueue.h
    CommandQueue/VKCommandQueue.h
    CommandQueue/VKCommandQueue.cpp

    CommandBuffer/ICommandBuffer.h
    CommandBuffer/VKCommandBuffer.h
    CommandBuffer/VKCommandBuffer.cpp

    Common/BaseTypes.h
    Common/VKConfig.h
    Common/VKConfig.cpp

    Device/IDevice.h
    Device/VKDevice.h
    Device/VKDevice.cpp

    Fence/IFence.h
    Fence/VKFence.h
    Fence/VKFence.cpp

    Image/IImage.h
    Image/IImage.cpp
    Image/ImageEnums.h
    Image/ImageFormat.h
    Image/ImageFormat.cpp
    Image/ImageSubresource.h
    Image/VKImage.h
    Image/VKImage.cpp
    Image/VKImageFormat.h
    
    Instance/IInstance.h
    Instance/IInstance.cpp
    Instance/VKInstance.h
    Instance/VKInstance.cpp

    Memory/IDeviceMemory.h
    Memory/VKDeviceMemory.h
    Memory/VKDeviceMemory.cpp

    Resource/IResource.h
    Resource/ResourceState.h

    SwapChain/ISwapChain.h
    SwapChain/VKSwapChain.h
    SwapChain/VKSwapChain.cpp
)

add_library(FeGPU STATIC ${SRC})

target_include_directories(FeGPU PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(FeGPU vulkan glfw FeCore)

get_property("TARGET_SOURCE_FILES" TARGET FeGPU PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
