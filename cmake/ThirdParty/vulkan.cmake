if (WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif()
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XCB_KHR)
endif()

add_subdirectory(${FE_THIRD_PARTY_DIR}/volk)
set_target_properties(volk PROPERTIES FOLDER "ThirdParty")


add_subdirectory(${FE_THIRD_PARTY_DIR}/VulkanMemoryAllocator)
set_target_properties(VulkanMemoryAllocator PROPERTIES FOLDER "ThirdParty")
