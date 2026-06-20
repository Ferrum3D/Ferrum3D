set(PUBLIC_HEADERS
    Public/FerrumCli/CommandLine.h
)

set(PRIVATE_SOURCES
    Private/FerrumCli/App.cpp
    Private/FerrumCli/App.h
)

set(GENERATED_SOURCES
    Reflection.gen.cpp
)

add_executable(FerrumCli
    ${PUBLIC_HEADERS}
    ${PRIVATE_SOURCES}
    ${GENERATED_SOURCES}
    main.cpp
)

fe_configure_target(FerrumCli)

target_include_directories(FerrumCli PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Public")
target_include_directories(FerrumCli PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Private")
target_compile_definitions(FerrumCli PRIVATE FE_FERRUM_VERSION="${PROJECT_VERSION}")

set_target_properties(FerrumCli PROPERTIES
    FOLDER "Tools"
    OUTPUT_NAME "ferrum"
)

target_link_libraries(FerrumCli
    AssetBuilder
    FeCore
    FeFramework
    FeGraphics
    FeGraphicsCore
)

get_property("TARGET_SOURCE_FILES" TARGET FerrumCli PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
