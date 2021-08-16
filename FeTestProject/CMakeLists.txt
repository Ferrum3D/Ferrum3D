add_executable(FeTestProject main.cpp VertexShader.h PixelShader.h)

fe_configure_target(FeTestProject)

set_target_properties(FeTestProject PROPERTIES FOLDER "Samples")
target_link_libraries(FeTestProject FeCore FeGPU)
