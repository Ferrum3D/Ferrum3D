set(SRC
    # FERRUM_SORT_LINES BEGIN
    Images/ImageAssetStorage.cpp
    Module.cpp
    # FERRUM_SORT_LINES END
    )

add_library(OsmiumAssetsBindings SHARED ${SRC})

set_target_properties(OsmiumAssetsBindings PROPERTIES FOLDER "Modules/Osmium")
target_link_libraries(OsmiumAssetsBindings OsmiumAssets OsmiumBindings)

get_property("TARGET_SOURCE_FILES" TARGET OsmiumAssetsBindings PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
