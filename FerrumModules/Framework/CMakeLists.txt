set(PUBLIC_HEADERS
    Public/Framework/Application/Core/PlatformApplication.h
    Public/Framework/Application/Core/PlatformEvent.h
    Public/Framework/Application/Core/PlatformMonitor.h
    Public/Framework/Application/Core/PlatformWindow.h

    Public/Framework/Application/Application.h

    Public/Framework/Input/Core/Keys.h

    Public/Framework/Module.h
)

set(COMMON_SOURCES
    Private/Framework/Module.cpp
)

set(WINDOWS_SOURCES
    Private/Framework/Application/Platform/Windows/PlatformApplication.cpp
    Private/Framework/Application/Platform/Windows/PlatformApplication.h
    Private/Framework/Application/Platform/Windows/PlatformWindow.cpp
    Private/Framework/Application/Platform/Windows/PlatformWindow.h
)

add_library(FeFramework SHARED ${PUBLIC_HEADERS} ${COMMON_SOURCES} ${WINDOWS_SOURCES})

fe_configure_target(FeFramework)

target_include_directories(FeFramework PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FeFramework PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")

set_target_properties(FeFramework PROPERTIES FOLDER "Modules/Framework")
target_link_libraries(FeFramework FeCore TracyClient)

get_property("TARGET_SOURCE_FILES" TARGET FeFramework PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
