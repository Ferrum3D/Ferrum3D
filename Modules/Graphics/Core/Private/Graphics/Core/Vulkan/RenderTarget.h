#pragma once
#include <Graphics/Core/RenderTarget.h>
#include <Graphics/Core/Vulkan/Image.h>

namespace FE::Graphics::Vulkan
{
    struct RenderTarget final
        : public Core::RenderTarget
        , public Image
    {
        FE_RTTI_Class(RenderTarget, "63C5B0A3-02D2-4B20-B271-DDD7AF4EF767");

        ~RenderTarget() override;

        static RenderTarget* Create(Core::Device* device);

        void InitInternal(VmaAllocator allocator, Env::Name name, const Core::ImageDesc& desc);
        void InitInternal(Env::Name name, const Core::ImageDesc& desc, VkImage image);

        const Core::ImageDesc& GetDesc() const override;

    private:
        explicit RenderTarget(Core::Device* device);
    };

    FE_ENABLE_NATIVE_CAST(RenderTarget);
} // namespace FE::Graphics::Vulkan
