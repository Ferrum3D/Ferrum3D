cmake_minimum_required(VERSION 3.17.0)

project(Ferrum3D VERSION 0.1.0
	DESCRIPTION "Ferrum3D Game Engine"
	LANGUAGES CXX C
)

enable_testing()
set(FE_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
list(PREPEND CMAKE_MODULE_PATH "${FE_PROJECT_ROOT}/cmake")

include(Config)
include(Dependencies)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(FerrumCore)
add_subdirectory(FerrumModules)
add_subdirectory(Samples)
