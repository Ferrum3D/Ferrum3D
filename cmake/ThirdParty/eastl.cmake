CPMAddPackage(
    NAME EASTL
    GITHUB_REPOSITORY electronicarts/EASTL
    VERSION 3.21.12
    GIT_TAG 3.21.12
    DOWNLOAD_ONLY YES
    GIT_SUBMODULES_RECURSE FALSE
)


add_definitions(-DEASTL_USER_CONFIG_HEADER=<${FE_PROJECT_ROOT}/FerrumCore/FeCore/Base/EASTLConfig.h>)
add_subdirectory(${EASTL_SOURCE_DIR})
set_target_properties(EASTL PROPERTIES FOLDER "ThirdParty")
