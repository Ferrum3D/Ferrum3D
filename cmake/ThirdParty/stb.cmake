add_library(stb INTERFACE)
target_include_directories(stb INTERFACE "${FE_PROJECT_ROOT}/ThirdParty/stb")
set_target_properties(stb PROPERTIES FOLDER "ThirdParty")
