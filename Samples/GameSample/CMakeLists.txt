file(GLOB_RECURSE SOURCES
        CONFIGURE_DEPENDS
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "*.cpp"
        "*.h"
)

add_executable(GameSample ${SOURCES})

fe_configure_target(GameSample)

target_compile_definitions(GameSample PRIVATE
    FE_GAME_SAMPLE_ASSET_DIR="${CMAKE_CURRENT_SOURCE_DIR}/Assets"
    FE_GAME_SAMPLE_RUNTIME_DIR="${CMAKE_CURRENT_BINARY_DIR}")

set_target_properties(GameSample PROPERTIES FOLDER "Samples")
target_link_libraries(GameSample FeCore FeGameFramework FeGraphics)
