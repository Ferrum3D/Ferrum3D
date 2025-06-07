set(TRACY_FIBERS ON CACHE BOOL "" FORCE)
set(TRACY_STATIC OFF CACHE BOOL "" FORCE)
add_subdirectory(${FE_THIRD_PARTY_DIR}/tracy)


set_target_properties(TracyClient PROPERTIES FOLDER "ThirdParty")
