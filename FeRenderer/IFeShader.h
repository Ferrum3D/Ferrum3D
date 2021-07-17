#pragma once
#include <FeCore/Utils/CoreUtils.h>
#include <iostream>

namespace FE
{
    // clang-format off
	FE_ENUM(FeShaderType)
	{
		None = 0,
		Vertex = 0x0001,
		Pixel = 0x0002,
		Geometry = 0x0004,
		Hull = 0x0008,
		Domain = 0x0010,
		Compute = 0x0020
	};
    // clang-format on

    FE_ENUM_TO_STR(FeShaderType)
    {
        FE_ENUM_STR_CASE(FeShaderType::None);
        FE_ENUM_STR_CASE(FeShaderType::Vertex);
        FE_ENUM_STR_CASE(FeShaderType::Pixel);
        FE_ENUM_STR_CASE(FeShaderType::Geometry);
        FE_ENUM_STR_CASE(FeShaderType::Hull);
        FE_ENUM_STR_CASE(FeShaderType::Domain);
        FE_ENUM_STR_CASE(FeShaderType::Compute);
        FE_ENUM_STR_CASE_DEF(FeShaderType);
    }

    struct FeShaderLoadDesc
    {
        std::string Name;
        std::string Path;
        FeShaderType Type;
    };

    class IFeShader
    {
    public:
        virtual const char* GetName() const  = 0;
        virtual FeShaderType GetType() const = 0;
    };
} // namespace FE
