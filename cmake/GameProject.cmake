# All game and game-specific editor code belongs in the shared library directory.
function(fe_add_game_project NAME)
    cmake_parse_arguments(GAME "" "LIBRARY_DIRECTORY;GAME_MAIN;EDITOR_MAIN;ASSET_DIRECTORY;FOLDER" "LINK_LIBRARIES" ${ARGN})
    foreach(REQUIRED LIBRARY_DIRECTORY GAME_MAIN EDITOR_MAIN ASSET_DIRECTORY)
        if(NOT GAME_${REQUIRED})
            message(FATAL_ERROR "fe_add_game_project(${NAME}): ${REQUIRED} is required")
        endif()
    endforeach()
    foreach(MAIN ${GAME_GAME_MAIN} ${GAME_EDITOR_MAIN})
        get_filename_component(MAIN_NAME "${MAIN}" NAME)
        if(NOT MAIN_NAME STREQUAL "main.cpp")
            message(FATAL_ERROR "fe_add_game_project(${NAME}): launcher source must be named main.cpp")
        endif()
    endforeach()

    get_filename_component(LIBRARY_DIRECTORY "${GAME_LIBRARY_DIRECTORY}" ABSOLUTE)
    get_filename_component(ASSET_DIRECTORY "${GAME_ASSET_DIRECTORY}" ABSOLUTE)
    file(GLOB_RECURSE LIBRARY_SOURCES CONFIGURE_DEPENDS "${LIBRARY_DIRECTORY}/*.cpp" "${LIBRARY_DIRECTORY}/*.h")
    add_library(${NAME}_Lib STATIC ${LIBRARY_SOURCES})
    fe_configure_target(${NAME}_Lib)
    target_include_directories(${NAME}_Lib PUBLIC "${LIBRARY_DIRECTORY}/Public")
    target_include_directories(${NAME}_Lib PRIVATE "${LIBRARY_DIRECTORY}/Private")
    target_link_libraries(${NAME}_Lib PUBLIC FeGameFramework ${GAME_LINK_LIBRARIES})
    target_compile_definitions(${NAME}_Lib PRIVATE
        FE_GAME_ASSET_DIRECTORY="${ASSET_DIRECTORY}"
        FE_GAME_RUNTIME_DIRECTORY="${CMAKE_CURRENT_BINARY_DIR}")

    # Runtime shader sources are copied at configure time; assets are still cooked manually with FerrumCli.
    foreach(SHADER_ROOT
            "${FE_PROJECT_ROOT}/Modules/Graphics/Framework/Shaders"
            "${FE_PROJECT_ROOT}/Modules/GameFramework/Shaders")
        file(GLOB_RECURSE SHADERS CONFIGURE_DEPENDS LIST_DIRECTORIES false RELATIVE "${SHADER_ROOT}"
            "${SHADER_ROOT}/*.h" "${SHADER_ROOT}/*.hlsl" "${SHADER_ROOT}/*.hlsli")
        foreach(SHADER ${SHADERS})
            configure_file("${SHADER_ROOT}/${SHADER}" "${CMAKE_CURRENT_BINARY_DIR}/Shaders/${SHADER}" COPYONLY)
        endforeach()
    endforeach()

    configure_file("${FE_PROJECT_ROOT}/ThirdParty/nerd-fonts/SymbolsNerdFontMono-Regular.ttf"
        "${CMAKE_CURRENT_BINARY_DIR}/Editor/Fonts/SymbolsNerdFontMono-Regular.ttf" COPYONLY)
    configure_file("${FE_PROJECT_ROOT}/ThirdParty/nerd-fonts/LICENSE"
        "${CMAKE_CURRENT_BINARY_DIR}/Editor/Fonts/LICENSE" COPYONLY)

    add_executable(${NAME} "${GAME_GAME_MAIN}")
    add_executable(${NAME}_Editor "${GAME_EDITOR_MAIN}")
    foreach(TARGET ${NAME} ${NAME}_Editor)
        fe_configure_target(${TARGET})
        target_link_libraries(${TARGET} PRIVATE ${NAME}_Lib)
    endforeach()

    if(GAME_FOLDER)
        set_target_properties(${NAME}_Lib ${NAME} ${NAME}_Editor PROPERTIES FOLDER "${GAME_FOLDER}")
    endif()
endfunction()
