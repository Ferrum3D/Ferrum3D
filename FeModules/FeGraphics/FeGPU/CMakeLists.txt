set(SRC
    Adapter/IAdapter.h
    Adapter/VKAdapter.h
    Adapter/VKAdapter.cpp

    Common/VKConfig.h
    Common/VKConfig.cpp

    Device/IDevice.h
    Device/VKDevice.h
    Device/VKDevice.cpp

    Fence/IFence.h
    Fence/VKFence.h
    Fence/VKFence.cpp
    
    Instance/IInstance.h
    Instance/IInstance.cpp
    Instance/VKInstance.h
    Instance/VKInstance.cpp
)

add_library(FeGPU STATIC ${SRC})

target_include_directories(FeGPU PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

target_link_libraries(FeGPU vulkan glfw FeCore)

get_property("TARGET_SOURCE_FILES" TARGET FeGPU PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
