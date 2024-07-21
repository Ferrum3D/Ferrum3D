CPMAddPackage(
    NAME SPIRV-Cross
    GITHUB_REPOSITORY KhronosGroup/SPIRV-Cross
    GIT_TAG vulkan-sdk-1.3.283.0
	OPTIONS
		"SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS ON"
)

set_target_properties(spirv-cross PROPERTIES FOLDER "${FE_PROJECT_ROOT}/ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-core PROPERTIES FOLDER "${FE_PROJECT_ROOT}/ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-c PROPERTIES FOLDER "${FE_PROJECT_ROOT}/ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-cpp PROPERTIES FOLDER "${FE_PROJECT_ROOT}/ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-glsl PROPERTIES FOLDER "${FE_PROJECT_ROOT}/ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-hlsl PROPERTIES FOLDER "${FE_PROJECT_ROOT}/ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-msl PROPERTIES FOLDER "${FE_PROJECT_ROOT}/ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-reflect PROPERTIES FOLDER "${FE_PROJECT_ROOT}/ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-util PROPERTIES FOLDER "${FE_PROJECT_ROOT}/ThirdParty/SPIRV-Cross")
