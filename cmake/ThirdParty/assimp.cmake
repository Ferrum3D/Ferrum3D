CPMAddPackage(
    NAME assimp
    GITHUB_REPOSITORY assimp/assimp
    VERSION 5.4.2
	OPTIONS
		"ASSIMP_BUILD_ASSIMP_TOOLS OFF"
		"ASSIMP_BUILD_TESTS OFF"
		"ASSIMP_NO_EXPORT ON"
)

set_target_properties(assimp PROPERTIES FOLDER "ThirdParty")
