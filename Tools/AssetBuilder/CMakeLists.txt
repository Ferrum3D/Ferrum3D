set(SOURCES
    App.cpp
    App.h
    Utils.cpp
    Utils.h
    TextureProcessor.cpp
    TextureProcessor.h
    MeshOptimization.cpp
    MeshOptimization.h
    ModelImporter.cpp
    ModelImporter.h
    ModelProcessor.cpp
    ModelProcessor.h

    main.cpp
)

add_executable(AssetBuilder ${SOURCES})
target_include_directories(AssetBuilder PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

fe_configure_target(AssetBuilder)

set_target_properties(AssetBuilder PROPERTIES FOLDER "Tools")
target_link_libraries(AssetBuilder
    FeCore
    FeFramework
    FeGraphics
    FeGraphicsCore
    compressonator
    meshoptimizer
    stb
    tiny_gltf
    rapidjson)
