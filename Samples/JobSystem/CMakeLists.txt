add_executable(JobSystemSample main.cpp)

fe_configure_target(JobSystemSample)

set_target_properties(JobSystemSample PROPERTIES FOLDER "Samples")
target_link_libraries(JobSystemSample FeCore)
