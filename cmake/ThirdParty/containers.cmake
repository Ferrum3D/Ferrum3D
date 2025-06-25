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

    ${FE_THIRD_PARTY_DIR}/small_vector/small_vector.natvis
)

add_library(EASTL STATIC ${EASTL_SOURCES})
target_include_directories(EASTL PUBLIC ${FE_THIRD_PARTY_DIR}/EASTL/include)
set_target_properties(EASTL PROPERTIES FOLDER "ThirdParty")

add_library(small_vector INTERFACE)

target_include_directories(small_vector INTERFACE "${FE_THIRD_PARTY_DIR}/small_vector/source/include")
set_target_properties(small_vector PROPERTIES FOLDER "ThirdParty")

add_library(tl_expected INTERFACE)
target_include_directories(tl_expected INTERFACE "${FE_THIRD_PARTY_DIR}/expected/include")

add_subdirectory(${FE_THIRD_PARTY_DIR}/unordered_dense)
set_target_properties(unordered_dense PROPERTIES FOLDER "ThirdParty")
