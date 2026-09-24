set(LUAU_ROOT "${FE_THIRD_PARTY_DIR}/luau")

function(fe_add_luau_library NAME)
    file(GLOB LUAU_SOURCES CONFIGURE_DEPENDS
        "${LUAU_ROOT}/${NAME}/src/*.cpp"
        "${LUAU_ROOT}/${NAME}/src/*.h"
        "${LUAU_ROOT}/${NAME}/include/*.h"
        "${LUAU_ROOT}/${NAME}/include/Luau/*.h")

    add_library(Luau.${NAME} STATIC ${LUAU_SOURCES})
    target_compile_features(Luau.${NAME} PUBLIC cxx_std_17)
    target_include_directories(Luau.${NAME} PUBLIC "${LUAU_ROOT}/${NAME}/include")
    set_target_properties(Luau.${NAME} PROPERTIES FOLDER "ThirdParty/Luau")
endfunction()

fe_add_luau_library(Common)
fe_add_luau_library(Ast)
fe_add_luau_library(Bytecode)
fe_add_luau_library(Compiler)
fe_add_luau_library(VM)

target_link_libraries(Luau.Ast PUBLIC Luau.Common)
target_link_libraries(Luau.Bytecode PUBLIC Luau.Common)
target_link_libraries(Luau.Compiler PUBLIC Luau.Ast Luau.Bytecode)
target_link_libraries(Luau.VM PUBLIC Luau.Common)

if(MSVC)
    target_compile_options(Luau.Ast PRIVATE /D_CRT_SECURE_NO_WARNINGS /we4018 /we4388 /MP)
    target_compile_options(Luau.VM PRIVATE /D_CRT_SECURE_NO_WARNINGS /we4018 /we4388 /MP)

    if(MSVC_VERSION GREATER_EQUAL 1924)
        set_source_files_properties("${LUAU_ROOT}/VM/src/lvmexecute.cpp" PROPERTIES COMPILE_FLAGS /d2ssa-pre-)
    endif()
else()
    target_compile_options(Luau.Ast PRIVATE -Wall -Wimplicit-fallthrough -Wsign-compare)
    target_compile_options(Luau.VM PRIVATE -Wall -Wimplicit-fallthrough -Wsign-compare -fno-math-errno)
endif()
