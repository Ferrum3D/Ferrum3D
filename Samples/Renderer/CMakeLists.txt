add_executable(RendererSample main.cpp)

fe_configure_target(RendererSample)

set_target_properties(RendererSample PROPERTIES FOLDER "Samples")
target_link_libraries(RendererSample FeCore FeFramework FeGraphics)
