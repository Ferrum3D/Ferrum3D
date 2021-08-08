set(SRC
	Common/TestCommon.h

	Math/Vector3.cpp
	
	Memory/StackAllocator.cpp
	
	Modules/Environment.cpp
	Modules/Singleton.cpp

	RTTI/RTTI.cpp
	
	Strings/Format.cpp
	Strings/String.cpp
	Utils/UUID.cpp

	main.cpp
)

add_executable(FeCoreTests ${SRC})

set_target_properties(FeCoreTests PROPERTIES FOLDER "Engine/FeCore")
target_link_libraries(FeCoreTests gtest gmock FeCore)

get_property("TARGET_SOURCE_FILES" TARGET FeCoreTests PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

gtest_discover_tests(FeCoreTests)
