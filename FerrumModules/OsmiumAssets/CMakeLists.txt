set(SRC
    # FERRUM_SORT_LINES BEGIN
    OsAssets/OsmiumAssets.h

    OsAssets/Images/ImageAssetLoader.cpp
    OsAssets/Images/ImageAssetLoader.h
    OsAssets/Images/ImageAssetStorage.cpp
    OsAssets/Images/ImageAssetStorage.h
    OsAssets/Images/StbImage.cpp
    OsAssets/Images/StbImage.h
    # FERRUM_SORT_LINES END
)

add_library(OsmiumAssets STATIC ${SRC})

fe_configure_target(OsmiumAssets)

target_include_directories(OsmiumAssets PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")

set_target_properties(OsmiumAssets PROPERTIES FOLDER "Modules/Osmium")
target_link_libraries(OsmiumAssets FeCore stb assimp)

get_property("TARGET_SOURCE_FILES" TARGET OsmiumAssets PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

add_subdirectory(Bindings)
