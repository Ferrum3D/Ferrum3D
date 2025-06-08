set(BC7ENC_SOURCES
    bc7enc/bc7enc.c
    bc7enc/bc7enc.h
)


add_executable(TextureCompressor main.cpp ${BC7ENC_SOURCES})
target_include_directories(TextureCompressor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

fe_configure_target(TextureCompressor)

set_target_properties(TextureCompressor PROPERTIES FOLDER "Tools")
target_link_libraries(TextureCompressor FeCore FeFramework FeGraphics FeGraphicsCore)
