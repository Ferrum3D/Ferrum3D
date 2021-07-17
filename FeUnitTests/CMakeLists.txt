cmake_minimum_required(VERSION 3.18.0)

project(FeUnitTests)

set(SRC
	main.cpp
	FeTestFramework.cpp
)
set(INCLUDE
	FeTestFramework.h
	
	FeVectorTests.h
	FeAllocatorTests.h
	FeAssetManagerTests.h
	FeECSTests.h
)

add_executable(FeUnitTests ${SRC} ${INCLUDE})
