add_executable(Uniforms main.cpp)

fe_configure_target(Uniforms)

set_target_properties(Uniforms PROPERTIES FOLDER "Samples")
target_link_libraries(Uniforms FeCore OsmiumGPU)
