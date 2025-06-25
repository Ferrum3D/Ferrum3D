if (WIN32)
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_WIN32_KHR)
elseif()
    set(VOLK_STATIC_DEFINES VK_USE_PLATFORM_XCB_KHR)
endif()

add_subdirectory(${FE_THIRD_PARTY_DIR}/volk)
set_target_properties(volk PROPERTIES FOLDER "ThirdParty")


add_subdirectory(${FE_THIRD_PARTY_DIR}/VulkanMemoryAllocator)
set_target_properties(VulkanMemoryAllocator PROPERTIES FOLDER "ThirdParty")

set(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS ON CACHE BOOL "" FORCE)
add_subdirectory(${FE_THIRD_PARTY_DIR}/SPIRV-Cross)

set_target_properties(spirv-cross PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-core PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-c PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-cpp PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-glsl PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-hlsl PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-msl PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-reflect PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
set_target_properties(spirv-cross-util PROPERTIES FOLDER "ThirdParty/SPIRV-Cross")
