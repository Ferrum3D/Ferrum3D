set(SRC
    FrameGraph/IFrameGraph.h

    Resource/IFrameGraphResource.h
    Resource/FrameGraphTexture.h
    Resource/FrameGraphTexture.cpp Resource/FrameGraphResourceBase.h)

add_library(OsmiumFrameGraph STATIC ${SRC})

fe_configure_target(OsmiumFrameGraph)

target_include_directories(OsmiumFrameGraph PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")

set_target_properties(OsmiumFrameGraph PROPERTIES FOLDER "Modules/Osmium")
target_link_libraries(OsmiumFrameGraph FeCore OsmiumGPU)

get_property("TARGET_SOURCE_FILES" TARGET OsmiumFrameGraph PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
