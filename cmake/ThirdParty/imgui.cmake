set(IMGUI_DIR "${FE_THIRD_PARTY_DIR}/imgui")

add_library(imgui STATIC
    ${IMGUI_DIR}/imconfig.h
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_internal.h
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imstb_rectpack.h
    ${IMGUI_DIR}/imstb_textedit.h
    ${IMGUI_DIR}/imstb_truetype.h)

target_include_directories(imgui PUBLIC ${IMGUI_DIR})
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS IMGUI_DISABLE_DEFAULT_ALLOCATORS)
set_target_properties(imgui PROPERTIES FOLDER "ThirdParty")
