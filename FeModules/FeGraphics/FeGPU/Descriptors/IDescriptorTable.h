#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeGPU/Shader/ShaderStage.h>
#include <FeGPU/Buffer/IBuffer.h>

namespace FE::GPU
{
    enum class ShaderResourceType
    {
        None,
        ConstantBuffer,
        TextureSRV,
        TextureUAV,
        BufferSRV,
        BufferUAV,
        Sampler,
        InputAttachment
    };

    struct DescriptorWriteBase
    {
        FE_STRUCT_RTTI(DescriptorWriteBase, "9FA1676E-C6BA-4F09-9BB5-BA0E137F17EB");

        UInt32 Binding = 0;
        UInt32 ArrayIndex = 0;
    };

    struct DescriptorWriteBuffer : DescriptorWriteBase
    {
        FE_STRUCT_RTTI(DescriptorWriteBuffer, "C19369F9-0A1C-4FEA-A517-4F47CA0E20BE");

        UInt32 Offset = 0;
        UInt32 Range = static_cast<UInt32>(-1);
        IBuffer* Buffer = nullptr;
    };

    struct DescriptorDesc
    {
        FE_STRUCT_RTTI(DescriptorDesc, "C4AAC4DD-3345-4EE7-B11D-9B2CD01EE31B");

        ShaderResourceType ResourceType;
        ShaderStageFlags Stage;
        UInt32 Count;
    };

    class IDescriptorTable : public IObject
    {
    public:
        FE_CLASS_RTTI(IDescriptorTable, "C10FCB1B-31C8-47EE-AE2F-E53463494B85");

        ~IDescriptorTable() override = default;

        virtual void Update(const DescriptorWriteBuffer& descriptorWriteBuffer) = 0;
    };
} // namespace FE::GPU
