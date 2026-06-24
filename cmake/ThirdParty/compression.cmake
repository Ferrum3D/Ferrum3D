set(LIBDEFLATE_SOURCES
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/deflate_compress.c
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/deflate_decompress.c
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/utils.c
  ${FE_THIRD_PARTY_DIR}/libdeflate/lib/x86/cpu_features.c
)

add_library(libdeflate STATIC ${LIBDEFLATE_SOURCES})
target_include_directories(libdeflate PUBLIC ${FE_THIRD_PARTY_DIR}/libdeflate)
set_target_properties(libdeflate PROPERTIES FOLDER "ThirdParty")

file(GLOB ZSTD_SOURCES CONFIGURE_DEPENDS
  ${FE_THIRD_PARTY_DIR}/zstd/lib/common/*.c
  ${FE_THIRD_PARTY_DIR}/zstd/lib/compress/*.c
  ${FE_THIRD_PARTY_DIR}/zstd/lib/decompress/*.c
)

add_library(libzstd_static STATIC ${ZSTD_SOURCES})
target_include_directories(libzstd_static
  PUBLIC ${FE_THIRD_PARTY_DIR}/zstd/lib
  PRIVATE ${FE_THIRD_PARTY_DIR}/zstd/lib/common
)
target_compile_definitions(libzstd_static PRIVATE
  XXH_NAMESPACE=ZSTD_
  ZSTD_LEGACY_SUPPORT=0
)
set_target_properties(libzstd_static PROPERTIES FOLDER "ThirdParty")
