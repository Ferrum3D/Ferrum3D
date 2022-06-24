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
    DeviceObjects/ImageView.cpp
    DeviceObjects/Instance.cpp
    DeviceObjects/RenderPass.cpp
    DeviceObjects/RenderPass.h
    DeviceObjects/SwapChain.cpp

    Shaders/ShaderCompiler.cpp
    Shaders/ShaderModule.cpp
    Shaders/ShaderModule.h

    WindowSystem/Window.cpp
    WindowSystem/Window.h
    # FERRUM_SORT_LINES END
)

add_library(OsmiumBindings SHARED ${SRC})

set_target_properties(OsmiumBindings PROPERTIES FOLDER "Modules/Osmium")
target_link_libraries(OsmiumBindings OsmiumGPU FeCoreBindings)

get_property("TARGET_SOURCE_FILES" TARGET OsmiumBindings PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
