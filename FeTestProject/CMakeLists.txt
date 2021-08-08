add_executable(FeTestProject main.cpp)

fe_configure_target(FeTestProject)

set_target_properties(FeTestProject PROPERTIES FOLDER "Samples")
target_link_libraries(FeTestProject FeCore FeGPU)
