add_executable(Triangle main.cpp)

fe_configure_target(Triangle)

set_target_properties(Triangle PROPERTIES FOLDER "Samples")
target_link_libraries(Triangle FeCore OsGPU)
