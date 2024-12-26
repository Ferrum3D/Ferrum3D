set(SRC
    Common/TestCommon.h

    Containers/BitSet.cpp
    Containers/LRUCacheMap.cpp
    Containers/SegmentedVector.cpp
    Containers/SparseSet.cpp

    Math/Matrix4x4.cpp
    Math/Vector3.cpp
    Math/Vector4.cpp

    Modules/Environment.cpp
    Modules/ServiceLocator.cpp

    RTTI/RTTI.cpp

    Strings/Format.cpp
    Strings/String.cpp

    Time/DateTime.cpp

    Utils/UUID.cpp

    main.cpp
)

add_executable(FeCoreTests ${SRC})

target_include_directories(FeCoreTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

set_target_properties(FeCoreTests PROPERTIES FOLDER "Engine/FeCore")
target_link_libraries(FeCoreTests gtest gmock FeCore)

get_property("TARGET_SOURCE_FILES" TARGET FeCoreTests PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

include(GoogleTest)
gtest_discover_tests(FeCoreTests)
