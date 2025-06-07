add_library(dxil SHARED IMPORTED)
set_target_properties(dxil PROPERTIES
    IMPORTED_IMPLIB ${FE_THIRD_PARTY_DIR}/dxc/lib/x64/dxil.lib
    IMPORTED_LOCATION ${FE_THIRD_PARTY_DIR}/dxc/bin/x64/dxil.dll
)

add_library(dxc SHARED IMPORTED)
set_target_properties(dxc PROPERTIES
    IMPORTED_IMPLIB ${FE_THIRD_PARTY_DIR}/dxc/lib/x64/dxcompiler.lib
    IMPORTED_LOCATION ${FE_THIRD_PARTY_DIR}/dxc/bin/x64/dxcompiler.dll
)


target_include_directories(dxc INTERFACE "${FE_THIRD_PARTY_DIR}/dxc/inc")

set_target_properties(dxil PROPERTIES FOLDER "ThirdParty")
set_target_properties(dxc PROPERTIES FOLDER "ThirdParty")
