#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Texture.h>

namespace FE::Graphics::Vulkan
{
    FE_DECLARE_VULKAN_OBJECT_POOL(Texture);


    Texture::~Texture()
    {
        Shutdown(NativeCast(m_device));
    }


    Texture* Texture::Create(Core::Device* device)
    {
        return Rc<Texture>::Allocate(&GTexturePool, [device](void* memory) {
            return new (memory) Texture(device);
        });
    }


    void Texture::InitInternal(const VmaAllocator allocator, const Env::Name name, const Core::ImageDesc& desc)
    {
        constexpr VkImageUsageFlags usage =
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        Image::InitInternal(NativeCast(m_device), name.c_str(), allocator, usage, desc);
    }


    const Core::ImageDesc& Texture::GetDesc() const
    {
        return m_desc;
    }


    Texture::Texture(Core::Device* device)
    {
        m_device = device;
        m_type = Core::ResourceType::kTexture;
        Register();
    }
} // namespace FE::Graphics::Vulkan
