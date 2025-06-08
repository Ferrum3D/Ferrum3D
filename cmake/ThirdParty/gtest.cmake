mark_as_advanced(
    BUILD_GMOCK BUILD_GTEST
    gmock_build_tests gtest_build_samples gtest_build_tests
    gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
add_subdirectory(${FE_THIRD_PARTY_DIR}/googletest)


set_target_properties(gtest      PROPERTIES FOLDER "ThirdParty")
set_target_properties(gtest_main PROPERTIES FOLDER "ThirdParty")
set_target_properties(gmock      PROPERTIES FOLDER "ThirdParty")
set_target_properties(gmock_main PROPERTIES FOLDER "ThirdParty")
