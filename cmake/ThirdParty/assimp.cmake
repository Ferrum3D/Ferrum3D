set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_NO_EXPORT ON)

add_subdirectory(${FE_PROJECT_ROOT}/ThirdParty/assimp)
set_target_properties(assimp PROPERTIES FOLDER "ThirdParty")
