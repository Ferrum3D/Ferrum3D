cmake_minimum_required(VERSION 3.18.0)

project(FerrumUnitTests)

set(SRC
	main.cpp
)
set(INCLUDE
	FeTestFramework.h
	FeTestFramework.cpp
	
	FeVectorTests.h
	FeAllocatorTests.h
	FeAssetManagerTests.h
)

add_executable(FerrumUnitTests ${SRC} ${INCLUDE})
