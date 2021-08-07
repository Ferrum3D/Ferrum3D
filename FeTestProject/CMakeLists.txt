add_executable(FeTestProject main.cpp)

fe_configure_target(FeTestProject)

target_link_libraries(FeTestProject FeCore FeGPU)
