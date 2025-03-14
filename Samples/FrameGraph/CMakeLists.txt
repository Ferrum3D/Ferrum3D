add_executable(FrameGraph main.cpp)

fe_configure_target(FrameGraph)

set_target_properties(FrameGraph PROPERTIES FOLDER "Samples")
target_link_libraries(FrameGraph FeCore FeFramework FeGraphicsCore)
