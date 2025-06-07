add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE "${FE_THIRD_PARTY_DIR}/rapidjson/include")

set_target_properties(rapidjson PROPERTIES FOLDER "ThirdParty")
