cmake_minimum_required(VERSION 3.18.0)

project(FerrumRenderer)

set(SRC
	FeRenderer.cpp
	FeWindow.cpp
	FeGraphicsDevice.cpp
)
set(INCLUDE
	FeRenderer.h
	FeWindow.h
	IFeWindow.h
	FeRenderAPI.h
	IFeGraphicsDevice.h
	FeGraphicsDevice.h
	FeRenderInternal.h
)

add_library(FerrumRenderer SHARED ${SRC} ${INCLUDE})

target_compile_definitions(FerrumRenderer PRIVATE FERRUMRENDERER_EXPORTS)
