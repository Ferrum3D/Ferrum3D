add_library(utf8proc STATIC
    ${FE_THIRD_PARTY_DIR}/utf8proc/utf8proc.c
    ${FE_THIRD_PARTY_DIR}/utf8proc/utf8proc.h)


target_include_directories(utf8proc PUBLIC ${FE_THIRD_PARTY_DIR}/utf8proc)
target_compile_definitions(utf8proc PUBLIC UTF8PROC_STATIC)
set_target_properties(utf8proc PROPERTIES FOLDER "ThirdParty")
