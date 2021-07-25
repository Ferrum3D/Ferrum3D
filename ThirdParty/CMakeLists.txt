cmake_minimum_required(VERSION 3.21.0)

set(CMAKE_CXX_STANDARD 14)

add_subdirectory("glfw")
add_subdirectory("DiligentEngine")

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory("googletest")

mark_as_advanced(
    BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
    gmock_build_tests gtest_build_samples gtest_build_tests
    gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
)

set_target_properties(gtest      PROPERTIES FOLDER ThirdParty)
set_target_properties(gtest_main PROPERTIES FOLDER ThirdParty)
set_target_properties(gmock      PROPERTIES FOLDER ThirdParty)
set_target_properties(gmock_main PROPERTIES FOLDER ThirdParty)
set_target_properties(glfw       PROPERTIES FOLDER ThirdParty)

set(CMAKE_CXX_STANDARD 17)
