cmake_minimum_required(VERSION 3.21.0)

set(SRC
	FeRenderer.cpp
	FeWindow.cpp
	FeGraphicsDevice.cpp
	FeShader.cpp
	FeVertexLayout.cpp
	FeTexture.cpp
	FeRenderContext.cpp
	FeRenderTarget.cpp
	FePipelineStateBuilder.cpp
	FeRenderPassBuilder.cpp
	FeRenderPassResources.cpp
)
set(INCLUDE
	FeRenderAPI.h
	FeRenderer.h
	FeWindow.h
	FeGraphicsDevice.h
	FeRenderInternal.h
	FeShader.h
	FeVertexLayout.h
	FeTexture.h
	FeRenderContext.h
	FeRenderTarget.h
	FePipelineStateBuilder.h
	FeFrameGraphResource.h
	FeRenderPassBuilder.h
	FeRenderPassResources.h
	
	IFeRenderPass.h
	IFeWindow.h
	IFeGraphicsDevice.h
	IFeShader.h
	IFeVertexLayout.h
	IFeTexture.h
	IFeRenderContext.h
	IFeRenderTarget.h
	IFePipelineStateBuilder.h
)

add_library(FeRenderer SHARED ${SRC} ${INCLUDE})

target_compile_definitions(FeRenderer PRIVATE FERRUMRENDERER_EXPORTS)
