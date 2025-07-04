﻿set(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS ON CACHE BOOL "" FORCE)
add_subdirectory(${FE_THIRD_PARTY_DIR}/SPIRV-Cross)

set_target_properties(spirv-cross PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-core PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-c PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-cpp PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-glsl PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-hlsl PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-msl PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-reflect PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-util PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
