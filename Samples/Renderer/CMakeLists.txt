file(GLOB_RECURSE SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.cpp"
    "*.h"
)

add_executable(RendererSample ${SOURCES})

fe_configure_target(RendererSample)

target_compile_definitions(RendererSample PRIVATE FE_RENDERER_SAMPLE_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets")

set_target_properties(RendererSample PROPERTIES FOLDER "Samples")
target_link_libraries(RendererSample FeCore FeFramework FeGraphics)

set(PALETTE_SHADER_SOURCE Assets/source/Shaders/Materials/BunnyVariants.ps.hlsl)
set_source_files_properties(${PALETTE_SHADER_SOURCE} PROPERTIES HEADER_FILE_ONLY TRUE)
target_sources(RendererSample PRIVATE ${PALETTE_SHADER_SOURCE})
configure_file(${PALETTE_SHADER_SOURCE} Shaders/Shaders/Materials/BunnyVariants.ps.hlsl COPYONLY)
