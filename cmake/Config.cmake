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

add_compile_definitions(TRACY_DELAYED_INIT)
add_compile_definitions(TRACY_MANUAL_LIFETIME)

function(fe_configure_target TARGET)
    if(FE_COMPILER_MSVC)
        target_compile_options(${TARGET} PRIVATE
            /EHs-c- /D_HAS_EXCEPTIONS=0 /fp:fast
            /Zc:preprocessor /arch:AVX /utf-8
            /W4 /WX /wd4324 /wd4201 /wd4127 /wd4373
            $<$<CONFIG:Debug>:/d2Obforceinline>)
    else()
        target_compile_options(${TARGET} PRIVATE -fno-exceptions -Wall -Werror -mavx -ffast-math
			-Wno-deprecated-builtins -Wno-language-extension-token)
    endif()
endfunction()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
