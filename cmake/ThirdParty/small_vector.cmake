add_library(small_vector INTERFACE)

target_include_directories(small_vector INTERFACE "${FE_THIRD_PARTY_DIR}/small_vector/source/include")
set_target_properties(small_vector PROPERTIES FOLDER "ThirdParty")
