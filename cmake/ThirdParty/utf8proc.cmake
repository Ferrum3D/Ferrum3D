CPMAddPackage(
    NAME utf8proc
    GITHUB_REPOSITORY JuliaStrings/utf8proc
    VERSION 2.9.0
    OPTIONS
          "UTF8PROC_INSTALL OFF"
          "UTF8PROC_ENABLE_TESTING OFF"
)


set_target_properties(utf8proc PROPERTIES FOLDER "ThirdParty")
