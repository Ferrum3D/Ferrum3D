#pragma once
#include <HAL/ShaderResourceType.h>
#include <HAL/ShaderStage.h>

namespace FE::Graphics::HAL
{
    class Buffer;
    class ImageView;
    class Sampler;

    struct DescriptorWriteBuffer
    {
        inline DescriptorWriteBuffer() = default;

        inline explicit DescriptorWriteBuffer(Buffer* buffer, uint32_t offset = 0, uint32_t range = static_cast<uint32_t>(-1),
                                              uint32_t binding = 0, uint32_t arrayIndex = 0)
            : Buffer(buffer)
            , Binding(binding)
            , ArrayIndex(arrayIndex)
            , Offset(offset)
            , Range(range)
        {
        }

        Buffer* Buffer = nullptr;
        uint32_t Binding = 0;
        uint32_t ArrayIndex = 0;
        uint32_t Offset = 0;
        uint32_t Range = static_cast<uint32_t>(-1);
    };

    struct DescriptorWriteImage
    {
        inline DescriptorWriteImage() = default;

        inline explicit DescriptorWriteImage(ImageView* imageView, uint32_t binding = 0, uint32_t arrayIndex = 0)
            : View(imageView)
            , Binding(binding)
            , ArrayIndex(arrayIndex)
        {
        }

        ImageView* View = nullptr;
        uint32_t Binding = 0;
        uint32_t ArrayIndex = 0;
    };

    struct DescriptorWriteSampler
    {
        inline DescriptorWriteSampler() = default;

        inline explicit DescriptorWriteSampler(Sampler* sampler, uint32_t binding = 0, uint32_t arrayIndex = 0)
            : Sampler(sampler)
            , Binding(binding)
            , ArrayIndex(arrayIndex)
        {
        }

        Sampler* Sampler = nullptr;
        uint32_t Binding = 0;
        uint32_t ArrayIndex = 0;
    };

    struct DescriptorDesc
    {
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
} // namespace FE::Graphics::HAL

FE_MAKE_HASHABLE(FE::Graphics::HAL::DescriptorDesc, , value.ResourceType, value.Stage, value.Count);
