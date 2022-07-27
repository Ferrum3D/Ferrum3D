set(SRC
    # FERRUM_SORT_LINES BEGIN
    Assets/AssetManager.cpp
    Assets/AssetRegistry.cpp

    Console/FeLog.cpp

    Containers/ByteBuffer.cpp

    ECS/ArchetypeChunk.cpp
    ECS/CallbackSystem.cpp
    ECS/EntityQuery.cpp
    ECS/EntityRegistry.cpp
    ECS/World.cpp

    EventBus/FrameEventBus.cpp

    Math/Matrix4x4F.cpp
    Math/Quaternion.cpp

    Modules/Engine.cpp

    Utils/UUID.cpp
    # FERRUM_SORT_LINES END
)

add_library(FeCoreBindings SHARED ${SRC})

set_target_properties(FeCoreBindings PROPERTIES FOLDER "Engine/FeCore")
target_link_libraries(FeCoreBindings FeCore)

get_property("TARGET_SOURCE_FILES" TARGET FeCoreBindings PROPERTY SOURCES)
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})
