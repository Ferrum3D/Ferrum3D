set(SHADER_SOURCES
    Shaders/Common.hlsli
    Shaders/Shader.as.hlsl
    Shaders/Shader.cs.hlsl
    Shaders/Shader.ms.hlsl
    Shaders/Shader.ps.hlsl
    Shaders/ShaderMesh.ps.hlsl
    Shaders/Shader.vs.hlsl
)

set_source_files_properties(${SHADER_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(FrameGraph ${SHADER_SOURCES} main.cpp)

fe_configure_target(FrameGraph)

set_target_properties(FrameGraph PROPERTIES FOLDER "Samples")
target_link_libraries(FrameGraph FeCore FeFramework FeGraphics)
