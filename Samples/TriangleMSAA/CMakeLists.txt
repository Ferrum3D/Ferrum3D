add_executable(TriangleMSAA main.cpp)

fe_configure_target(TriangleMSAA)

set_target_properties(TriangleMSAA PROPERTIES FOLDER "Samples")
target_link_libraries(TriangleMSAA FeCore OsGPU)
