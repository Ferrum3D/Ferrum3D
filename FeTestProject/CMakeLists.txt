add_executable(FeTestProject main.cpp)

fe_configure_target(FeTestProject)

set_target_properties(FeTestProject PROPERTIES FOLDER "Samples")
target_link_libraries(FeTestProject FeCore OsmiumGPU)

configure_file(Shaders/PixelShader.hlsl ${CMAKE_BINARY_DIR}/Assets/TestProject/Shaders/PixelShader.hlsl COPYONLY)
configure_file(Shaders/VertexShader.hlsl ${CMAKE_BINARY_DIR}/Assets/TestProject/Shaders/VertexShader.hlsl COPYONLY)
