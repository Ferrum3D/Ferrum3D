#pragma once
#include <Graphics/Core/Sampler.h>
#include <Graphics/Core/Vulkan/Base/Config.h>

namespace FE::Graphics::Vulkan
{
    struct Device;

    struct SamplerFactory final
    {
        using ObjectType = VkSampler;
        using ObjectDesc = Core::SamplerState;

        static VkSampler Create(const Device* device, const Core::SamplerState& samplerState);
        static void Destroy(const Device* device, VkSampler sampler);
    };
} // namespace FE::Graphics::Vulkan
