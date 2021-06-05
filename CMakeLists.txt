cmake_minimum_required(VERSION 3.18.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
project(Ferrum3D)

add_subdirectory(FerrumCore)
add_subdirectory(FerrumRenderer)
add_subdirectory(FerrumTestProject)

add_subdirectory(External)

set_property(TARGET FerrumCore PROPERTY FOLDER "Ferrum3D")
set_property(TARGET FerrumRenderer PROPERTY FOLDER "Ferrum3D")
set_property(TARGET FerrumTestProject PROPERTY FOLDER "Ferrum3D")

# renderer dependencies
target_include_directories(FerrumRenderer
PRIVATE
	"FerrumCore/include"
	"External/glfw/include"
	"External/DiligentEngine"
)
target_link_libraries(FerrumRenderer
PRIVATE
    Diligent-GraphicsEngineD3D11-shared
    Diligent-GraphicsEngineOpenGL-shared
    Diligent-GraphicsEngineD3D12-shared
    Diligent-GraphicsEngineVk-shared
	glfw
	FerrumCore
)

# test project dependencies
target_include_directories(FerrumTestProject
PRIVATE
	"FerrumCore/include"
	"FerrumRenderer/include"
)
target_link_libraries(FerrumTestProject
	FerrumCore 
	FerrumRenderer
)
