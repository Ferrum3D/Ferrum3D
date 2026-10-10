file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
add_executable(FeFrameworkTests ${SOURCES})
fe_configure_target(FeFrameworkTests)
target_include_directories(FeFrameworkTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Private"
)
target_link_libraries(FeFrameworkTests PRIVATE FeFramework AssetBuilder gtest gmock)
set_target_properties(FeFrameworkTests PROPERTIES FOLDER "Modules/Framework")
include(GoogleTest)
gtest_discover_tests(FeFrameworkTests)

target_compile_definitions(FeFrameworkTests PRIVATE
    FE_FRAMEWORK_CORE_FIXTURES="${PROJECT_SOURCE_DIR}/FerrumCore/Tests/Fixtures/Artifacts"
    FE_ENTITY_ASSET_TEST_OUTPUT="${PROJECT_BINARY_DIR}/EntityAssetTests"
)
