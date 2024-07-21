set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)


mark_as_advanced(
    BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
    gmock_build_tests gtest_build_samples gtest_build_tests
    gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
)


CPMAddPackage(
    NAME googletest
    GITHUB_REPOSITORY google/googletest
    GIT_TAG release-1.12.1
    VERSION 1.12.1
    OPTIONS
        "INSTALL_GTEST OFF"
        "gtest_force_shared_crt"
)


set_target_properties(gtest      PROPERTIES FOLDER "ThirdParty")
set_target_properties(gtest_main PROPERTIES FOLDER "ThirdParty")
set_target_properties(gmock      PROPERTIES FOLDER "ThirdParty")
set_target_properties(gmock_main PROPERTIES FOLDER "ThirdParty")
