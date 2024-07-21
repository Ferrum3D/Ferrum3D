if (WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif()
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XCB_KHR)
endif()

CPMAddPackage(
    NAME volk
    GITHUB_REPOSITORY zeux/volk
    VERSION 1.3.270
    GIT_TAG 1.3.270
)

set_target_properties(volk PROPERTIES FOLDER "ThirdParty")
