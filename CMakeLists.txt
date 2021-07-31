cmake_minimum_required(VERSION 3.21.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_compile_definitions(FE_SSE3_SUPPORTED=1)
add_compile_definitions(FE_SSE41_SUPPORTED=1)

project(Ferrum3D VERSION 0.0.1
	DESCRIPTION "Ferrum3D Game Engine"
	LANGUAGES CXX
)

enable_testing()
include(GoogleTest)

add_subdirectory(FeCore)
add_subdirectory(FeRenderer)
add_subdirectory(FeTestProject)
add_subdirectory(FeAssetCompiler)

add_subdirectory(ThirdParty)

target_compile_definitions(Diligent-ShaderTools PRIVATE _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)

set_property(TARGET FeCore 			PROPERTY FOLDER "Ferrum3D/FeCore")
set_property(TARGET FeCoreTests 	PROPERTY FOLDER "Ferrum3D/FeCore")
set_property(TARGET FeRenderer 		PROPERTY FOLDER "Ferrum3D/FeRenderer")
set_property(TARGET FeTestProject 	PROPERTY FOLDER "Ferrum3D/FeTestProject")
set_property(TARGET FeAssetCompiler PROPERTY FOLDER "Ferrum3D/FeAssetCompiler")

# renderer dependencies
target_include_directories(FeRenderer
PRIVATE
	"./"
	"FeCore"
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
	"FeCore/FeCore"
	"ThirdParty/entt"
	"ThirdParty/rpmalloc"
)

target_link_libraries(FeCore
	rpmalloc
)

target_include_directories(FeCoreTests
PRIVATE
	"FeCore"
)
target_link_libraries(FeCoreTests
	FeCore
)

# test project dependencies
target_include_directories(FeTestProject
PRIVATE
	"./"
	"FeCore"
)
target_link_libraries(FeTestProject
	FeCore
	# FeRenderer
)

# asset compiler dependencies
target_include_directories(FeAssetCompiler
PRIVATE
	"FeCore"
)
target_link_libraries(FeAssetCompiler
	FeCore
)
