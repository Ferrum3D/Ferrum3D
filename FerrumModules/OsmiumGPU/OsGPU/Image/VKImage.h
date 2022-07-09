#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Image/ImageBase.h>

namespace FE::Osmium
{
    class VKDevice;
    class VKDeviceMemory;

    class VKImage : public ImageBase
    {
        VKDevice* m_Device;
        Shared<VKDeviceMemory> m_Memory;
        bool m_Owned = false;

    public:
        VkImage Image;
        ImageDesc Desc;

        FE_CLASS_RTTI(VKImage, "9726C432-92C1-489C-9623-55330B3530E8");

        explicit VKImage(VKDevice& dev);
        VKImage(VKDevice& dev, const ImageDesc& desc);
        ~VKImage() override;

        const ImageDesc& GetDesc() override;
        Shared<IImageView> CreateView(ImageAspectFlags aspectFlags) override;

        void AllocateMemory(MemoryType type) override;
        void BindMemory(const Shared<IDeviceMemory>& memory, UInt64 offset) override;
    };
} // namespace FE::Osmium
