cmake_minimum_required(VERSION 3.18.0)

project(FerrumRenderer)

set(SRC
	FeRenderer.cpp
	FeWindow.cpp
	FeGraphicsDevice.cpp
)
set(INCLUDE
	FeRenderAPI.h
	FeRenderer.h
	FeWindow.h
	FeGraphicsDevice.h
	FeRenderInternal.h
	FeShader.cpp
	FeShader.h
	
	IFeRenderPass.h
	IFeWindow.h
	IFeGraphicsDevice.h
	IFeShader.h
)

add_library(FerrumRenderer SHARED ${SRC} ${INCLUDE})

target_compile_definitions(FerrumRenderer PRIVATE FERRUMRENDERER_EXPORTS)
