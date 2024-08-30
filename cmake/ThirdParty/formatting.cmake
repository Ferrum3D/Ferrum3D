CPMAddPackage(
    NAME jeaiii_itoa
    GITHUB_REPOSITORY jeaiii/itoa
	GIT_TAG main
)

add_library(jeaiii_itoa INTERFACE)
target_include_directories(jeaiii_itoa INTERFACE ${jeaiii_itoa_SOURCE_DIR}/include)
set_target_properties(jeaiii_itoa PROPERTIES FOLDER "ThirdParty")

CPMAddPackage("gh:jk-jeon/dragonbox#1.1.3")
