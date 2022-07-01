set(SRC
    # FERRUM_SORT_LINES BEGIN
    Images/ImageAssetStorage.cpp

    Module.cpp
    # FERRUM_SORT_LINES END
)

add_library(OsAssetsBindings SHARED ${SRC})

set_target_properties(OsAssetsBindings PROPERTIES FOLDER "Modules/Osmium")
target_link_libraries(OsAssetsBindings OsAssets OsGPUBindings)

get_property("TARGET_SOURCE_FILES" TARGET OsAssetsBindings PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
