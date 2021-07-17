#pragma once
#include "IFeShader.h"
#include "FeGraphicsDevice.h"

namespace FE
{
	struct FeShaderDesc
	{
		FeShaderType Type;
		const char* SourceCode;
		const char* Name;
		DL::IShaderSourceInputStreamFactory* ShaderSourceFactory;
		DL::IRenderDevice* Device;
	};

	class FeShader : public IFeShader
	{
		std::string m_Name;
		FeShaderType m_Type;

		DL::RefCntAutoPtr<DL::IShader> m_Handle;
	public:
		FeShader(const FeShaderDesc& desc);

		virtual const char* GetName() const override;
		virtual FeShaderType GetType() const override;

		DL::RefCntAutoPtr<DL::IShader> GetHandle();
	};
}
