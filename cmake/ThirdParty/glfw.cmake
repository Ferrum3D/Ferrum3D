add_subdirectory(${FE_PROJECT_ROOT}/ThirdParty/glfw)
set_target_properties(glfw PROPERTIES FOLDER "ThirdParty")
target_include_directories(glfw INTERFACE "${FE_PROJECT_ROOT}/ThirdParty/glfw/include")
