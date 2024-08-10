#pragma once
#include <FeCore/Memory/Memory.h>
#include <OsGPU/Image/ImageFormat.h>

namespace FE::Osmium
{
    struct ShaderInputAttribute
    {
        FE_RTTI_Base(ShaderInputAttribute, "99AEC831-F140-45C1-9FA3-C3B5FBE1C098");

        uint32_t Location;
        FE::String ShaderSemantic;
        Format ElementFormat;
    };

    class IShaderReflection : public Memory::RefCountedObjectBase
    {
    public:
        FE_RTTI_Class(IShaderReflection, "9ECFF14F-1D5A-4997-B6D5-735E935A9D64");

        ~IShaderReflection() override = default;

        virtual eastl::vector<ShaderInputAttribute> GetInputAttributes() = 0;
        virtual uint32_t GetInputAttributeLocation(StringSlice semantic) = 0;
    };
} // namespace FE::Osmium
