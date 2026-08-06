file(GLOB_RECURSE PUBLIC_HEADERS
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Public/*.h"
)

file(GLOB_RECURSE COMMON_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Private/*.cpp"
    "Private/*.h"
)

file(GLOB_RECURSE WINDOWS_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "Private/Framework/Application/Platform/Windows/*.cpp"
    "Private/Framework/Application/Platform/Windows/*.h"
)
list(REMOVE_ITEM COMMON_SOURCES ${WINDOWS_SOURCES})

file(GLOB GENERATED_SOURCES
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.gen.cpp"
)

add_library(FeFramework STATIC ${PUBLIC_HEADERS} ${COMMON_SOURCES} ${WINDOWS_SOURCES} ${GENERATED_SOURCES})

fe_configure_target(FeFramework)

target_include_directories(FeFramework PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FeFramework PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")

set_target_properties(FeFramework PROPERTIES FOLDER "Modules/Framework")
target_link_libraries(FeFramework FeCore TracyClient)

get_property("TARGET_SOURCE_FILES" TARGET FeFramework PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
