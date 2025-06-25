#pragma once
#include <Graphics/Core/DeviceObject.h>
#include <Graphics/Core/ShaderSpecialization.h>

namespace FE::Graphics::Core
{
    struct ShaderHandle final : public TypedHandle<ShaderHandle, uint32_t>
    {
    };


    struct ShaderLibrary : public DeviceObject
    {
        FE_RTTI_Class(ShaderLibrary, "BE44FCFD-5540-49F6-AECE-569BE88A8450");

        virtual ShaderHandle GetShader(Env::Name name, Env::Name defines) = 0;
    };
} // namespace FE::Graphics::Core
