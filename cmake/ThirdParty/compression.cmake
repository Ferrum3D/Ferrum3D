CPMAddPackage(
    NAME bc7enc
    GITHUB_REPOSITORY richgel999/bc7enc_rdo
	GIT_TAG master
	DOWNLOAD_ONLY YES
)


add_library(bc7enc STATIC
	${bc7enc_SOURCE_DIR}/bc7enc.h
	${bc7enc_SOURCE_DIR}/bc7enc.cpp
)

target_include_directories(bc7enc PUBLIC ${bc7enc_SOURCE_DIR})
set_target_properties(bc7enc PROPERTIES FOLDER "ThirdParty")
