add_definitions(-DEASTL_USER_CONFIG_HEADER=<${FE_PROJECT_ROOT}/FerrumCore/Private/FeCore/Base/EASTLConfig.h>)

set(EASTL_SOURCES
    ${FE_THIRD_PARTY_DIR}/EASTL/source/allocator_eastl.cpp
    ${FE_THIRD_PARTY_DIR}/EASTL/source/assert.cpp
    ${FE_THIRD_PARTY_DIR}/EASTL/source/atomic.cpp
    ${FE_THIRD_PARTY_DIR}/EASTL/source/fixed_pool.cpp
    ${FE_THIRD_PARTY_DIR}/EASTL/source/hashtable.cpp
    ${FE_THIRD_PARTY_DIR}/EASTL/source/intrusive_list.cpp
    ${FE_THIRD_PARTY_DIR}/EASTL/source/numeric_limits.cpp
    ${FE_THIRD_PARTY_DIR}/EASTL/source/red_black_tree.cpp
    ${FE_THIRD_PARTY_DIR}/EASTL/source/string.cpp
    ${FE_THIRD_PARTY_DIR}/EASTL/source/thread_support.cpp
    ${FE_THIRD_PARTY_DIR}/EASTL/EASTL.natvis
)

add_library(EASTL STATIC ${EASTL_SOURCES})
target_include_directories(EASTL PUBLIC ${FE_THIRD_PARTY_DIR}/EASTL/include)
set_target_properties(EASTL PROPERTIES FOLDER "ThirdParty")
