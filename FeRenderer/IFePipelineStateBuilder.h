#pragma once
#include "IFeShader.h"
#include "IFeVertexLayout.h"
#include <FeCore/Utils/CoreUtils.h>

namespace FE
{
    // clang-format off
	FE_ENUM(FePipelineType)
	{
		None,
		Graphics,
		Compute
	};

	FE_ENUM(FeVertexLayoutType)
	{
		Default,
		Count
	};
	// clang format on
	
	struct FeShaderProgramDesc
	{
		std::shared_ptr<IFeShader> PixelShader;
		std::shared_ptr<IFeShader> VertexShader;
	};

	class IFePipelineStateBuilder
	{
	public:
		virtual void SetPipelineType(FePipelineType type) = 0;
		virtual void SetShaders(const FeShaderProgramDesc& desc) = 0;
		virtual std::shared_ptr<IFeVertexLayout> GetVertexLayout(FeVertexLayoutType type) = 0;
		virtual void SetVertexLayout(IFeVertexLayout* layout) = 0;
	};
}
