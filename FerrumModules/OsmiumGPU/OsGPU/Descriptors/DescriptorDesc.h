#pragma once
#include <OsGPU/Resource/ShaderResourceType.h>
#include <OsGPU/Shader/ShaderStage.h>

namespace FE::Osmium
{
    class IBuffer;
    class IImageView;
    class ISampler;

    struct DescriptorWriteBuffer
    {
        FE_STRUCT_RTTI(DescriptorWriteBuffer, "C19369F9-0A1C-4FEA-A517-4F47CA0E20BE");

        inline DescriptorWriteBuffer() = default;

        inline explicit DescriptorWriteBuffer(IBuffer* buffer, UInt32 offset = 0, UInt32 range = static_cast<UInt32>(-1),
                                              UInt32 binding = 0, UInt32 arrayIndex = 0)
            : Buffer(buffer)
            , Binding(binding)
            , ArrayIndex(arrayIndex)
            , Offset(offset)
            , Range(range)
        {
        }

        IBuffer* Buffer   = nullptr;
        UInt32 Binding    = 0;
        UInt32 ArrayIndex = 0;
        UInt32 Offset     = 0;
        UInt32 Range      = static_cast<UInt32>(-1);
    };

    struct DescriptorWriteImage
    {
        FE_STRUCT_RTTI(DescriptorWriteImage, "680ED3D3-DB75-474C-8EA8-1F5F9090F5DE");

        inline DescriptorWriteImage() = default;

        inline explicit DescriptorWriteImage(IImageView* imageView, UInt32 binding = 0, UInt32 arrayIndex = 0)
            : View(imageView)
            , Binding(binding)
            , ArrayIndex(arrayIndex)
        {
        }

        IImageView* View  = nullptr;
        UInt32 Binding    = 0;
        UInt32 ArrayIndex = 0;
    };

    struct DescriptorWriteSampler
    {
        FE_STRUCT_RTTI(DescriptorWriteSampler, "7690B5AF-5653-47F7-B12D-0D2F9E4E9DB2");

        inline DescriptorWriteSampler() = default;

        inline explicit DescriptorWriteSampler(ISampler* sampler, UInt32 binding = 0, UInt32 arrayIndex = 0)
            : Sampler(sampler)
            , Binding(binding)
            , ArrayIndex(arrayIndex)
        {
        }

        ISampler* Sampler = nullptr;
        UInt32 Binding    = 0;
        UInt32 ArrayIndex = 0;
    };

    struct DescriptorDesc
    {
        FE_STRUCT_RTTI(DescriptorDesc, "C4AAC4DD-3345-4EE7-B11D-9B2CD01EE31B");

        inline DescriptorDesc() = default;

        inline DescriptorDesc(ShaderResourceType resourceType, ShaderStageFlags stage, UInt32 count)
            : ResourceType(resourceType)
            , Stage(stage)
            , Count(count)
        {
        }

        ShaderResourceType ResourceType;
        ShaderStageFlags Stage;
        UInt32 Count;
    };
} // namespace FE::Osmium

FE_MAKE_HASHABLE(FE::Osmium::DescriptorDesc, , value.ResourceType, value.Stage, value.Count);
