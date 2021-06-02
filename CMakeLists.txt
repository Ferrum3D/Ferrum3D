cmake_minimum_required(VERSION 3.18.0)
project(Ferrum3D)

add_subdirectory(FerrumCore)
add_subdirectory(FerrumRenderer)
add_subdirectory(FerrumTestProject)

# renderer dependencies
target_link_libraries(FerrumRenderer FerrumCore)
target_include_directories(FerrumRenderer PRIVATE "FerrumCore/")

# test project dependencies
target_link_libraries(FerrumTestProject FerrumCore FerrumRenderer)
target_include_directories(FerrumTestProject PRIVATE "FerrumCore/" "FerrumRenderer/")
