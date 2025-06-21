set(SHADER_SOURCES
    Shaders/Shader.ps.hlsl
    Shaders/Shader.vs.hlsl
)

set_source_files_properties(${SHADER_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(FrameGraph ${SHADER_SOURCES} main.cpp)

fe_configure_target(FrameGraph)

set_target_properties(FrameGraph PROPERTIES FOLDER "Samples")
target_link_libraries(FrameGraph FeCore FeFramework FeGraphics)
