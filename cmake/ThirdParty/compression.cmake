set(LIBDEFLATE_SOURCES
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/adler32.c
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/crc32.c
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/deflate_compress.c
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/deflate_decompress.c
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/gdeflate_compress.c
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/gdeflate_decompress.c
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/zlib_compress.c
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/zlib_decompress.c
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/utils.c
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/x86/cpu_features.c
)

set(LIBDEFLATE_HEADERS
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/adler32_vec_template.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/bt_matchfinder.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/crc32_table.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/crc32_vec_template.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/decompress_template.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/deflate_compress.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/deflate_constants.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/hc_matchfinder.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/lib_common.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/matchfinder_common.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/unaligned.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/x86/adler32_impl.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/x86/cpu_features.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/x86/crc32_impl.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/x86/crc32_pclmul_template.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/x86/decompress_impl.h
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/x86/matchfinder_impl.h
)

add_library(libdeflate STATIC ${LIBDEFLATE_SOURCES} ${LIBDEFLATE_HEADERS})
target_include_directories(libdeflate PUBLIC ${FE_THIRD_PARTY_DIR}/libdeflate)
set_target_properties(libdeflate PROPERTIES FOLDER "ThirdParty")
