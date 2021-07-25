cmake_minimum_required(VERSION 3.21.0)

set(SRC
	Math/Vector3.cpp
	
	Memory/StackAllocator.cpp
	
	Modules/Environment.cpp
)

add_executable(FeCoreTests ${SRC})
target_link_libraries(FeCoreTests gtest gmock gtest_main)

get_property("TARGET_SOURCE_FILES" TARGET FeCoreTests PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

gtest_discover_tests(FeCoreTests)
