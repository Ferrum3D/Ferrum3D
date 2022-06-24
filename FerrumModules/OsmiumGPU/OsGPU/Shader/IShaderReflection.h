#pragma once
#include <FeCore/Memory/Memory.h>
#include <OsGPU/Image/ImageFormat.h>

namespace FE::Osmium
{
    struct ShaderInputAttribute
    {
        FE_STRUCT_RTTI(ShaderInputAttribute, "99AEC831-F140-45C1-9FA3-C3B5FBE1C098");

        UInt32 Location;
        FE::String ShaderSemantic;
        Format ElementFormat;
    };

    class IShaderReflection : public IObject
    {
    public:
        FE_CLASS_RTTI(IShaderReflection, "9ECFF14F-1D5A-4997-B6D5-735E935A9D64");

        ~IShaderReflection() override = default;

        virtual Vector<ShaderInputAttribute> GetInputAttributes()      = 0;
        virtual UInt32 GetInputAttributeLocation(StringSlice semantic) = 0;
    };
} // namespace FE::Osmium
