CPMAddPackage(
    NAME mimalloc
    GITHUB_REPOSITORY microsoft/mimalloc
    VERSION 2.1.4
    OPTIONS
          "MI_BUILD_TESTS OFF"
          "MI_BUILD_SHARED ON"
          "MI_BUILD_STATIC OFF"
)


set_target_properties(mimalloc PROPERTIES FOLDER "ThirdParty")
