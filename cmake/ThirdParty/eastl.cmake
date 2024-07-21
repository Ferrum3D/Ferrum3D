CPMAddPackage(
    NAME EASTL
    GITHUB_REPOSITORY electronicarts/EASTL
    VERSION 3.21.12
    GIT_TAG 3.21.12
    DOWNLOAD_ONLY YES
    GIT_SUBMODULES "test/packages/EABase"
    GIT_SUBMODULES_RECURSE FALSE
)

add_library(EASTL INTERFACE)
target_include_directories(EASTL INTERFACE "${EASTL_SOURCE_DIR}/include")
target_include_directories(EASTL INTERFACE "${EASTL_SOURCE_DIR}/test/packages/EABase/include/Common")

set_target_properties(EASTL PROPERTIES FOLDER "ThirdParty")
add_definitions(-DEASTL_USER_CONFIG_HEADER=<FeCore/Base/EASTLConfig.h>)
