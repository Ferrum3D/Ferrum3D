CPMAddPackage(
    NAME rapidjson
    GITHUB_REPOSITORY Tencent/rapidjson
    VERSION 1.1.0
	DOWNLOAD_ONLY YES
)

add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE "${rapidjson_SOURCE_DIR}/include")

set_target_properties(rapidjson PROPERTIES FOLDER "ThirdParty")
