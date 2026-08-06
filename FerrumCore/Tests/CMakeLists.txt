file(GLOB_RECURSE SRC
    CONFIGURE_DEPENDS
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.cpp"
    "*.h"
)

add_executable(FeCoreTests ${SRC})

fe_configure_target(FeCoreTests)

target_include_directories(FeCoreTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${CMAKE_CURRENT_SOURCE_DIR}/../Private")

set_target_properties(FeCoreTests PROPERTIES FOLDER "Core")
target_link_libraries(FeCoreTests gtest gmock FeCore)

get_property("TARGET_SOURCE_FILES" TARGET FeCoreTests PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

include(GoogleTest)
gtest_discover_tests(FeCoreTests)
