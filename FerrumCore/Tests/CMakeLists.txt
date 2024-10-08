﻿set(SRC
    # FERRUM_SORT_LINES BEGIN
    Assets/AssetManager.cpp
    Assets/TestAssetLoader.h

    Common/TestCommon.h

    Containers/LRUCacheMap.cpp
    Containers/SegmentedVector.cpp
    Containers/SparseSet.cpp

    Math/Matrix4x4.cpp
    Math/Vector3.cpp

    Modules/Environment.cpp
    Modules/ServiceLocator.cpp

    RTTI/RTTI.cpp

    Strings/Format.cpp
    Strings/String.cpp

    Utils/Result.cpp
    Utils/UUID.cpp

    main.cpp
    # FERRUM_SORT_LINES END
)

add_executable(FeCoreTests ${SRC})

configure_file(Assets/TestAsset1.txt ${CMAKE_BINARY_DIR}/Assets/TestAsset1.txt COPYONLY)
configure_file(Assets/TestAsset2.txt ${CMAKE_BINARY_DIR}/Assets/TestAsset2.txt COPYONLY)

set_target_properties(FeCoreTests PROPERTIES FOLDER "Engine/FeCore")
target_link_libraries(FeCoreTests gtest gmock FeCore)

get_property("TARGET_SOURCE_FILES" TARGET FeCoreTests PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

include(GoogleTest)
gtest_discover_tests(FeCoreTests)
