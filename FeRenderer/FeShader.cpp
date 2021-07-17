#include <FeCore/Console/FeLog.h>
#include "FeShader.h"

namespace FE
{
	FeShader::FeShader(const FeShaderDesc& desc) {
		m_Name = desc.Name;
		m_Type = desc.Type;

		DL::ShaderCreateInfo info{};
		info.SourceLanguage = DL::SHADER_SOURCE_LANGUAGE_HLSL;
		info.Source = desc.SourceCode;
		info.pShaderSourceStreamFactory = desc.ShaderSourceFactory;

		info.Desc.ShaderType = (DL::SHADER_TYPE)m_Type;
		info.Desc.Name = desc.Name;
		info.EntryPoint = "main";

		desc.Device->CreateShader(info, &m_Handle);
		LogMsg("Created shader \"{}\" of type {}", m_Name, m_Type);
	}

	const char* FeShader::GetName() const {
		return nullptr;
	}

	FeShaderType FeShader::GetType() const {
		return FeShaderType();
	}

	DL::RefCntAutoPtr<DL::IShader> FeShader::GetHandle() {
		return m_Handle;
	}
}
