set(XXHASH_SOURCES
    ${FE_THIRD_PARTY_DIR}/xxhash/xxhash.c
    ${FE_THIRD_PARTY_DIR}/xxhash/xxhash.h
)

add_library(xxhash STATIC ${XXHASH_SOURCES})
target_include_directories(xxhash PUBLIC ${FE_THIRD_PARTY_DIR}/xxHash)
set_target_properties(xxhash PROPERTIES FOLDER "ThirdParty")
