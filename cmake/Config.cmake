if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(FE_COMPILER_CLANG ON)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(FE_COMPILER_GCC ON)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(FE_COMPILER_MSVC ON)
endif()

set(CMAKE_DEBUG_POSTFIX "")

if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
    add_compile_definitions(_ENABLE_EXTENDED_ALIGNED_STORAGE)
    set(CMAKE_USE_WIN32_THREADS_INIT ON)
endif()

option(FE_USE_SSE3 "Set this option to use SSE3 instructions in engine" ON)
option(FE_USE_SSE41 "Set this option to use SSE4.1 instructions in engine" ON)

if (FE_USE_SSE41)
    if (NOT FE_USE_SSE3)
        message(WARNING "SSE4.1 instructions are enabled, but SSE3 are not")
        message(NOTICE "SSE3 instructions will be enabled automatically")
        set(FE_USE_SSE3 ON)
    endif()
else()
    message(FATAL_ERROR "Builds without SSE4.1 enabled are not supported yet")
endif()

if (FE_USE_SSE3)
    add_compile_definitions(FE_SSE3_SUPPORTED=1)
endif()
if (FE_USE_SSE41)
    add_compile_definitions(FE_SSE41_SUPPORTED=1)
endif()

add_compile_definitions(TRACY_DELAYED_INIT)
add_compile_definitions(TRACY_MANUAL_LIFETIME)

function(fe_enable_sse_for_target SSE_TARGET)
    if (FE_USE_SSE41 AND NOT FE_COMPILER_MSVC)
        target_compile_options(${SSE_TARGET} PUBLIC -msse4.1)
    endif()
endfunction()

function(fe_configure_target TARGET)
    if(FE_COMPILER_MSVC)
        target_compile_options(${TARGET} PRIVATE /EHs-c- /D_HAS_EXCEPTIONS=0 /fp:fast
            /W4 /WX /wd4324 /wd4201)
    else()
        target_compile_options(${TARGET} PRIVATE -fno-exceptions -Wall -Werror
			-Wno-deprecated-builtins -Wno-language-extension-token)
    endif()
endfunction()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
