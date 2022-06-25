add_executable(Textures main.cpp)

fe_configure_target(Textures)

set_target_properties(Textures PROPERTIES FOLDER "Samples")
target_link_libraries(Textures FeCore OsmiumGPU OsmiumAssets)

configure_file(Shaders/PixelShader.hlsl ${CMAKE_BINARY_DIR}/Assets/Samples/Textures/Shaders/PixelShader.hlsl COPYONLY)
configure_file(Shaders/VertexShader.hlsl ${CMAKE_BINARY_DIR}/Assets/Samples/Textures/Shaders/VertexShader.hlsl COPYONLY)
configure_file(Textures/image.jpg ${CMAKE_BINARY_DIR}/Assets/Samples/Textures/Textures/image.jpg COPYONLY)
