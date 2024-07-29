CPMAddPackage(
    NAME Tracy
    GITHUB_REPOSITORY wolfpld/tracy
    VERSION 0.11.0
	OPTIONS "TRACY_FIBERS ON"
)

set_target_properties(TracyClient PROPERTIES FOLDER "ThirdParty")
