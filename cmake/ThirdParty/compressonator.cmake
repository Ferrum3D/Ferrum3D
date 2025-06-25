set(COMPRESSONATOR_SOURCES
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc1_encode_kernel.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc1_common_kernel.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc1_encode_kernel.cpp
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc2_encode_kernel.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc2_encode_kernel.cpp
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc3_encode_kernel.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc3_encode_kernel.cpp
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc4_encode_kernel.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc4_encode_kernel.cpp
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc5_encode_kernel.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc5_encode_kernel.cpp
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc6_encode_kernel.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc6_encode_kernel.cpp
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc7_encode_kernel.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc7_common_encoder.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bc7_encode_kernel.cpp
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bcn_common_kernel.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/bcn_common_api.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders/common_def.h

    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/source/cmp_core.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/source/cmp_core.cpp
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/source/cmp_math_vec4.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/source/cmp_math_func.h
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/source/core_simd_avx.cpp

    ${FE_THIRD_PARTY_DIR}/compressonator/applications/_libs/cmp_math/cpu_extensions.cpp
    ${FE_THIRD_PARTY_DIR}/compressonator/applications/_libs/cmp_math/cmp_math_common.cpp
)

add_library(compressonator STATIC ${COMPRESSONATOR_SOURCES})
target_include_directories(compressonator PUBLIC
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/shaders
    ${FE_THIRD_PARTY_DIR}/compressonator/cmp_core/source
    ${FE_THIRD_PARTY_DIR}/compressonator/applications/_libs/cmp_math
)

set_target_properties(compressonator PROPERTIES FOLDER "ThirdParty")
