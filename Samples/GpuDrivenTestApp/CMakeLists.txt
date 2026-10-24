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

set(SHADER_ROOT "${PROJECT_SOURCE_DIR}/Modules/Graphics/Framework/Shaders")
file(GLOB_RECURSE RUNTIME_SHADERS CONFIGURE_DEPENDS LIST_DIRECTORIES false RELATIVE "${SHADER_ROOT}"
    "${SHADER_ROOT}/*.h" "${SHADER_ROOT}/*.hlsl" "${SHADER_ROOT}/*.hlsli")
foreach(SHADER ${RUNTIME_SHADERS})
    configure_file("${SHADER_ROOT}/${SHADER}" "${CMAKE_CURRENT_BINARY_DIR}/Shaders/${SHADER}" COPYONLY)
endforeach()
