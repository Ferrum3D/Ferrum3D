add_library(jeaiii_itoa INTERFACE)
target_include_directories(jeaiii_itoa INTERFACE ${FE_THIRD_PARTY_DIR}/jeaiii_itoa/include)
set_target_properties(jeaiii_itoa PROPERTIES FOLDER "ThirdParty")

add_subdirectory(${FE_THIRD_PARTY_DIR}/dragonbox)
set_target_properties(dragonbox PROPERTIES FOLDER "ThirdParty")
