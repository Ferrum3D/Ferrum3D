add_executable(FeAssetCompiler main.cpp)

fe_configure_target(FeAssetCompiler)

set_target_properties(FeAssetCompiler PROPERTIES FOLDER "Engine/AssetCompiler")
target_link_libraries(FeAssetCompiler FeCore)
