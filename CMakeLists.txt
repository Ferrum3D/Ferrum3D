cmake_minimum_required(VERSION 3.18.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(Ferrum3D)

add_subdirectory(FeCore)
add_subdirectory(FeRenderer)
add_subdirectory(FeTestProject)
add_subdirectory(FeUnitTests)
add_subdirectory(FeAssetCompiler)

add_subdirectory(ThirdParty)

target_compile_definitions(Diligent-ShaderTools PRIVATE _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)

set_property(TARGET FeCore PROPERTY FOLDER "Ferrum3D")
set_property(TARGET FeRenderer PROPERTY FOLDER "Ferrum3D")
set_property(TARGET FeTestProject PROPERTY FOLDER "Ferrum3D")
set_property(TARGET FeUnitTests PROPERTY FOLDER "Ferrum3D")
set_property(TARGET FeAssetCompiler PROPERTY FOLDER "Ferrum3D")

# renderer dependencies
target_include_directories(FeRenderer
PRIVATE
	"./"
	"ThirdParty/glfw/include"
	"ThirdParty/DiligentEngine"
)
target_link_libraries(FeRenderer
PRIVATE
    Diligent-GraphicsEngineD3D11-shared
    Diligent-GraphicsEngineD3D12-shared
    Diligent-GraphicsEngineVk-shared
	glfw
	FeCore
)

# core dependencies
target_include_directories(FeCore
PUBLIC
	"FeCore"
	"ThirdParty/entt"
	"ThirdParty/utf8h"
)

# test project dependencies
target_include_directories(FeTestProject
PRIVATE
	"./"
)
target_link_libraries(FeTestProject
	FeCore
	FeRenderer
)

# unit tests project dependencies
target_include_directories(FeUnitTests
PRIVATE
	"./"
)
target_link_libraries(FeUnitTests
	FeCore
	FeRenderer
)

# asset compiler dependencies
target_include_directories(FeAssetCompiler
PRIVATE
	"./"
)
target_link_libraries(FeAssetCompiler
	FeCore
)
