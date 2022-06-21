add_executable(Uniforms main.cpp)

fe_configure_target(Uniforms)

set_target_properties(Uniforms PROPERTIES FOLDER "Samples")
target_link_libraries(Uniforms FeCore OsmiumGPU)

configure_file(Shaders/PixelShader.hlsl ${CMAKE_BINARY_DIR}/Assets/Samples/Uniforms/Shaders/PixelShader.hlsl COPYONLY)
configure_file(Shaders/VertexShader.hlsl ${CMAKE_BINARY_DIR}/Assets/Samples/Uniforms/Shaders/VertexShader.hlsl COPYONLY)
