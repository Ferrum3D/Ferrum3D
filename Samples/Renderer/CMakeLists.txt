file(GLOB_RECURSE SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.cpp"
    "*.h"
)

add_executable(RendererSample ${SOURCES})

fe_configure_target(RendererSample)

set_target_properties(RendererSample PROPERTIES FOLDER "Samples")
target_link_libraries(RendererSample FeCore FeFramework FeGraphics)
