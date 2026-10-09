set(GAME_SAMPLE_ASSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/Assets")
add_executable(GameSampleAssetBuilder BuildAssets.cpp SampleAssets.h)
fe_configure_target(GameSampleAssetBuilder)
target_link_libraries(GameSampleAssetBuilder PRIVATE AssetBuilder FeGameFramework)
target_compile_definitions(GameSampleAssetBuilder PRIVATE
    FE_GAME_SAMPLE_WORLD_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/Assets/World.asset"
    FE_GAME_SAMPLE_ASSET_DIR="${GAME_SAMPLE_ASSET_DIR}")
set_target_properties(GameSampleAssetBuilder PROPERTIES FOLDER "Samples/GameSample")

# Reuse the Renderer sample's cooked Helmet dependencies without duplicating source assets.
add_custom_target(GameSampleAssets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/Samples/Renderer/Assets/artifacts" "${GAME_SAMPLE_ASSET_DIR}/artifacts"
    COMMAND $<TARGET_FILE:GameSampleAssetBuilder>
    DEPENDS GameSampleAssetBuilder "${CMAKE_CURRENT_SOURCE_DIR}/Assets/World.asset"
    VERBATIM)
set_target_properties(GameSampleAssets PROPERTIES FOLDER "Samples/GameSample")
add_executable(GameSample main.cpp SampleAssets.h)
fe_configure_target(GameSample)
target_link_libraries(GameSample PRIVATE FeGameFramework FeGraphics)
target_compile_definitions(GameSample PRIVATE
    FE_GAME_SAMPLE_ASSET_DIR="${GAME_SAMPLE_ASSET_DIR}"
    FE_GAME_SAMPLE_RUNTIME_DIR="${CMAKE_CURRENT_BINARY_DIR}")
set_target_properties(GameSample PROPERTIES FOLDER "Samples/GameSample")
add_dependencies(GameSample GameSampleAssets)
