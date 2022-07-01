add_executable(Textures main.cpp)

fe_configure_target(Textures)

set_target_properties(Textures PROPERTIES FOLDER "Samples")
target_link_libraries(Textures FeCore OsGPU OsAssets)
