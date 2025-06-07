set(PUBLIC_HEADERS
    Public/Graphics/Assets/TextureAssetManager.h

    Public/Graphics/Module.h
)

set(COMMON_SOURCES
    Private/Graphics/Assets/TextureAssetManager.cpp
    Private/Graphics/Assets/TextureAssetManager.h

    Private/Graphics/Module.cpp
)

add_library(FeGraphics SHARED ${PUBLIC_HEADERS} ${COMMON_SOURCES})

fe_configure_target(FeGraphics)

target_include_directories(FeGraphics PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FeGraphics PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")

set_target_properties(FeGraphics PROPERTIES FOLDER "Modules/Graphics")
target_link_libraries(FeGraphics FeGraphicsCore bc7enc stb assimp TracyClient)

get_property("TARGET_SOURCE_FILES" TARGET FeGraphics PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
