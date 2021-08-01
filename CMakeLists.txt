cmake_minimum_required(VERSION 3.21.0)

project(Ferrum3D VERSION 0.0.1
	DESCRIPTION "Ferrum3D Game Engine"
	LANGUAGES CXX
)

enable_testing()
set(FE_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
list(PREPEND CMAKE_MODULE_PATH "${FE_PROJECT_ROOT}/cmake")

include(Config)
include(ThirdParty/entt)
include(ThirdParty/glfw)
include(ThirdParty/gtest)
include(ThirdParty/rpmalloc)

add_subdirectory(FeCore)
add_subdirectory(FeTestProject)
add_subdirectory(FeAssetCompiler)
add_subdirectory(FeModules)
