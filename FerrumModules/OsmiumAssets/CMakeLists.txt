set(SRC
    # FERRUM_SORT_LINES BEGIN
    OsAssets/OsmiumAssets.h

    OsAssets/Images/ImageAssetLoader.cpp
    OsAssets/Images/ImageAssetLoader.h
    OsAssets/Images/ImageAssetStorage.cpp
    OsAssets/Images/ImageAssetStorage.h
    OsAssets/Images/ImageLoaderImpl.cpp
    OsAssets/Images/ImageLoaderImpl.h

    OsAssets/Meshes/MeshAssetLoader.cpp
    OsAssets/Meshes/MeshAssetLoader.h
    OsAssets/Meshes/MeshAssetStorage.cpp
    OsAssets/Meshes/MeshAssetStorage.h
    OsAssets/Meshes/MeshLoaderImpl.cpp
    OsAssets/Meshes/MeshLoaderImpl.h
    # FERRUM_SORT_LINES END
)

add_library(OsAssets STATIC ${SRC})

fe_configure_target(OsAssets)

target_include_directories(OsAssets PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/")

set_target_properties(OsAssets PROPERTIES FOLDER "Modules/Osmium")
target_link_libraries(OsAssets OsGPU FeCore stb assimp)

get_property("TARGET_SOURCE_FILES" TARGET OsAssets PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

add_subdirectory(Bindings)
