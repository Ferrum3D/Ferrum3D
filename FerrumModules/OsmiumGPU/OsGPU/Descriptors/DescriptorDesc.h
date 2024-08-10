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
        FE_RTTI_Base(DescriptorWriteBuffer, "C19369F9-0A1C-4FEA-A517-4F47CA0E20BE");

        inline DescriptorWriteBuffer() = default;

        inline explicit DescriptorWriteBuffer(IBuffer* buffer, uint32_t offset = 0, uint32_t range = static_cast<uint32_t>(-1),
                                              uint32_t binding = 0, uint32_t arrayIndex = 0)
            : Buffer(buffer)
            , Binding(binding)
            , ArrayIndex(arrayIndex)
            , Offset(offset)
            , Range(range)
        {
        }

        IBuffer* Buffer   = nullptr;
        uint32_t Binding    = 0;
        uint32_t ArrayIndex = 0;
        uint32_t Offset     = 0;
        uint32_t Range      = static_cast<uint32_t>(-1);
    };

    struct DescriptorWriteImage
    {
        FE_RTTI_Base(DescriptorWriteImage, "680ED3D3-DB75-474C-8EA8-1F5F9090F5DE");

        inline DescriptorWriteImage() = default;

        inline explicit DescriptorWriteImage(IImageView* imageView, uint32_t binding = 0, uint32_t arrayIndex = 0)
            : View(imageView)
            , Binding(binding)
            , ArrayIndex(arrayIndex)
        {
        }

        IImageView* View  = nullptr;
        uint32_t Binding    = 0;
        uint32_t ArrayIndex = 0;
    };

    struct DescriptorWriteSampler
    {
        FE_RTTI_Base(DescriptorWriteSampler, "7690B5AF-5653-47F7-B12D-0D2F9E4E9DB2");

        inline DescriptorWriteSampler() = default;

        inline explicit DescriptorWriteSampler(ISampler* sampler, uint32_t binding = 0, uint32_t arrayIndex = 0)
            : Sampler(sampler)
            , Binding(binding)
            , ArrayIndex(arrayIndex)
        {
        }

        ISampler* Sampler = nullptr;
        uint32_t Binding    = 0;
        uint32_t ArrayIndex = 0;
    };

    struct DescriptorDesc
    {
        FE_RTTI_Base(DescriptorDesc, "C4AAC4DD-3345-4EE7-B11D-9B2CD01EE31B");

        inline DescriptorDesc() = default;

        inline DescriptorDesc(ShaderResourceType resourceType, ShaderStageFlags stage, uint32_t count)
            : ResourceType(resourceType)
            , Stage(stage)
            , Count(count)
        {
        }

        ShaderResourceType ResourceType;
        ShaderStageFlags Stage;
        uint32_t Count;
    };
} // namespace FE::Osmium

FE_MAKE_HASHABLE(FE::Osmium::DescriptorDesc, , value.ResourceType, value.Stage, value.Count);
