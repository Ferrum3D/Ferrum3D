add_executable(FeAssetCompiler main.cpp)

fe_configure_target(FeAssetCompiler)

set_target_properties(FeAssetCompiler PROPERTIES FOLDER "Engine/AssetCompiler")
set_target_properties(FeAssetCompiler PROPERTIES OUTPUT_NAME "feassetc")
target_link_libraries(FeAssetCompiler FeCore OsAssets rapidjson)
