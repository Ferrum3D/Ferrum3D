set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)

add_subdirectory(${FE_PROJECT_ROOT}/ThirdParty/glfw)
set_target_properties(glfw PROPERTIES FOLDER "ThirdParty")
target_include_directories(glfw INTERFACE "${FE_PROJECT_ROOT}/ThirdParty/glfw/include")
