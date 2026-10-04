file(GLOB_RECURSE SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.cpp"
    "*.h"
)

add_executable(GpuDrivenTestApp ${SOURCES})

fe_configure_target(GpuDrivenTestApp)

target_compile_definitions(GpuDrivenTestApp PRIVATE FE_GPU_DRIVEN_TEST_APP_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets")

set_target_properties(GpuDrivenTestApp PROPERTIES FOLDER "Samples")
target_link_libraries(GpuDrivenTestApp FeCore FeFramework FeGraphics)

set(TEST_SHADER_SOURCE Assets/source/Shaders/Materials/BunnyVariants.ps.hlsl)
set_source_files_properties(${TEST_SHADER_SOURCE} PROPERTIES HEADER_FILE_ONLY TRUE)
target_sources(GpuDrivenTestApp PRIVATE ${TEST_SHADER_SOURCE})
configure_file(${TEST_SHADER_SOURCE} Shaders/Shaders/Materials/BunnyVariants.ps.hlsl COPYONLY)
