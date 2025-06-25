set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(MI_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(MI_OVERRIDE OFF CACHE BOOL "" FORCE)
set(MI_WIN_REDIRECT OFF CACHE BOOL "" FORCE)


add_subdirectory(${FE_THIRD_PARTY_DIR}/mimalloc)
set_target_properties(mimalloc-static PROPERTIES FOLDER "ThirdParty")

add_library(DebugHeap STATIC ${FE_THIRD_PARTY_DIR}/ig-debugheap/DebugHeap.c ${FE_THIRD_PARTY_DIR}/ig-debugheap/DebugHeap.h)
set_target_properties(DebugHeap PROPERTIES FOLDER "ThirdParty")
target_include_directories(DebugHeap PUBLIC ${FE_THIRD_PARTY_DIR}/ig-debugheap)
