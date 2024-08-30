add_executable(Models main.cpp)

fe_configure_target(Models)

set_target_properties(Models PROPERTIES FOLDER "Samples")
target_link_libraries(Models FeCore FeGraphics)
