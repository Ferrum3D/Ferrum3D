add_library(rpmalloc STATIC
    "${FE_PROJECT_ROOT}/ThirdParty/rpmalloc/rpmalloc/rpmalloc.h"
    "${FE_PROJECT_ROOT}/ThirdParty/rpmalloc/rpmalloc/rpmalloc.c"
)

set_target_properties(rpmalloc PROPERTIES FOLDER "ThirdParty")
target_include_directories(rpmalloc INTERFACE "${FE_PROJECT_ROOT}/ThirdParty/rpmalloc")
