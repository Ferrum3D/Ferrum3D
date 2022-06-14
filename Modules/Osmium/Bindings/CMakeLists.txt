set(SRC
    Instance/Instance.cpp
    Adapter/Adapter.cpp
    Device/Device.cpp)

add_library(OsmiumBindings SHARED ${SRC})

set_target_properties(OsmiumBindings PROPERTIES FOLDER "Modules/Osmium")
target_link_libraries(OsmiumBindings OsmiumGPU FeCoreBindings)

get_property("TARGET_SOURCE_FILES" TARGET OsmiumBindings PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
