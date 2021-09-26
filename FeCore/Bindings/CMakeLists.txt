set(SRC
    Math/Matrix4x4F.cpp)

add_library(FeCoreBindings SHARED ${SRC})

set_target_properties(FeCoreBindings PROPERTIES FOLDER "Engine/FeCore")
target_link_libraries(FeCoreBindings FeCore)

get_property("TARGET_SOURCE_FILES" TARGET FeCoreBindings PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
