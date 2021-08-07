add_executable(FeAssetCompiler main.cpp)

fe_configure_target(FeAssetCompiler)

target_link_libraries(FeAssetCompiler FeCore)
