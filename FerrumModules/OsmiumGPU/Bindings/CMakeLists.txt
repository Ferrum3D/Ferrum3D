set(SRC
    # FERRUM_SORT_LINES BEGIN
    Common.h

    DeviceObjects/Adapter.cpp
    DeviceObjects/Buffer.cpp
    DeviceObjects/CommandBuffer.cpp
    DeviceObjects/CommandQueue.cpp
    DeviceObjects/DescriptorHeap.cpp
    DeviceObjects/DescriptorHeap.h
    DeviceObjects/DescriptorTable.cpp
    DeviceObjects/Device.cpp
    DeviceObjects/Fence.cpp
    DeviceObjects/Framebuffer.cpp
    DeviceObjects/Framebuffer.h
    DeviceObjects/GraphicsPipeline.cpp
    DeviceObjects/GraphicsPipeline.h
    DeviceObjects/Image.cpp
    DeviceObjects/ImageView.cpp
    DeviceObjects/Instance.cpp
    DeviceObjects/RenderPass.cpp
    DeviceObjects/RenderPass.h
    DeviceObjects/Sampler.cpp
    DeviceObjects/SwapChain.cpp
    DeviceObjects/TransientResourceHeap.cpp

    Module.cpp

    Shaders/ShaderCompiler.cpp
    Shaders/ShaderModule.cpp
    Shaders/ShaderModule.h

    WindowSystem/Window.cpp
    WindowSystem/Window.h
    # FERRUM_SORT_LINES END
)

add_library(OsGPUBindings SHARED ${SRC})

set_target_properties(OsGPUBindings PROPERTIES FOLDER "Modules/Osmium")
target_link_libraries(OsGPUBindings OsGPU FeCoreBindings)

get_property("TARGET_SOURCE_FILES" TARGET OsGPUBindings PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
