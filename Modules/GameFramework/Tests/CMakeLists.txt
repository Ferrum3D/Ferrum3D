file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp" "*.h")
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/ReflectionLink.cpp")
add_executable(FeGameFrameworkTests ${SOURCES})
fe_configure_target(FeGameFrameworkTests)
target_link_libraries(FeGameFrameworkTests PRIVATE FeGameFramework AssetBuilder gtest gmock)
target_include_directories(FeGameFrameworkTests PRIVATE "${PROJECT_SOURCE_DIR}/Modules/Graphics/Framework/Private")
set_target_properties(FeGameFrameworkTests PROPERTIES FOLDER "Modules/GameFramework")
include(GoogleTest)
gtest_discover_tests(FeGameFrameworkTests)

target_compile_definitions(FeGameFrameworkTests PRIVATE FE_GAME_ASSET_TEST_OUTPUT="${PROJECT_BINARY_DIR}/GameAssetTests")

# This executable has no typed module references, so it exercises archive retention through the generated anchor alone.
add_executable(FeGameFrameworkReflectionLinkTests ReflectionLink.cpp)
fe_configure_target(FeGameFrameworkReflectionLinkTests)
target_link_libraries(FeGameFrameworkReflectionLinkTests PRIVATE FeGameFramework)
set_target_properties(FeGameFrameworkReflectionLinkTests PROPERTIES FOLDER "Modules/GameFramework")
add_test(NAME FeGameFrameworkReflectionLinkTests COMMAND FeGameFrameworkReflectionLinkTests)
