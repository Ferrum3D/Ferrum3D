set(SRC
    DeviceObjects/Instance.cpp
    DeviceObjects/Adapter.cpp
    DeviceObjects/Device.cpp
    DeviceObjects/CommandQueue.cpp
    Shaders/ShaderCompiler.cpp
    WindowSystem/Window.cpp
    WindowSystem/Window.h
    DeviceObjects/SwapChain.cpp
    DeviceObjects/Buffer.cpp
    Shaders/ShaderModule.cpp
    Shaders/ShaderModule.h
    DeviceObjects/RenderPass.cpp
    DeviceObjects/RenderPass.h
    DeviceObjects/GraphicsPipeline.cpp
    DeviceObjects/GraphicsPipeline.h
    DeviceObjects/Fence.cpp DeviceObjects/ImageView.cpp)

add_library(OsmiumBindings SHARED ${SRC})

set_target_properties(OsmiumBindings PROPERTIES FOLDER "Modules/Osmium")
target_link_libraries(OsmiumBindings OsmiumGPU FeCoreBindings)

get_property("TARGET_SOURCE_FILES" TARGET OsmiumBindings PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
