CPMAddPackage(
    NAME glfw
    GITHUB_REPOSITORY glfw/glfw
    GIT_TAG 3.3.10
    VERSION 3.3.10
    OPTIONS
          "GLFW_BUILD_TESTS OFF"
          "GLFW_BUILD_EXAMPLES OFF"
          "GLFW_BULID_DOCS OFF"
          "GLFW_INSTALL OFF"
)


set_target_properties(glfw PROPERTIES FOLDER "ThirdParty")
