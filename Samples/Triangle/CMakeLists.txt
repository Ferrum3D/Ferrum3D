add_executable(Triangle main.cpp)

fe_configure_target(Triangle)

set_target_properties(Triangle PROPERTIES FOLDER "Samples")
target_link_libraries(Triangle FeCore OsmiumGPU)

configure_file(Shaders/PixelShader.hlsl ${CMAKE_BINARY_DIR}/Assets/Samples/Triangle/Shaders/PixelShader.hlsl COPYONLY)
configure_file(Shaders/VertexShader.hlsl ${CMAKE_BINARY_DIR}/Assets/Samples/Triangle/Shaders/VertexShader.hlsl COPYONLY)
