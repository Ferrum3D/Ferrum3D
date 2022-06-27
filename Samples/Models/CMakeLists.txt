add_executable(Models main.cpp)

fe_configure_target(Models)

set_target_properties(Models PROPERTIES FOLDER "Samples")
target_link_libraries(Models FeCore OsmiumGPU OsmiumAssets)

configure_file(Shaders/PixelShader.hlsl ${CMAKE_BINARY_DIR}/Assets/Samples/Models/Shaders/PixelShader.hlsl COPYONLY)
configure_file(Shaders/VertexShader.hlsl ${CMAKE_BINARY_DIR}/Assets/Samples/Models/Shaders/VertexShader.hlsl COPYONLY)
configure_file(Textures/image.jpg ${CMAKE_BINARY_DIR}/Assets/Samples/Textures/Models/image.jpg COPYONLY)
configure_file(FerrumAssetIndex ${CMAKE_BINARY_DIR}/Assets/Samples/Models/FerrumAssetIndex COPYONLY)
