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
        DeviceMemorySlice m_Memory;
        bool m_MemoryOwned = false;
        bool m_Owned = false;

    public:
        VkMemoryRequirements MemoryRequirements;
        VkImage Image;
        ImageDesc Desc;

        FE_RTTI_Class(VKImage, "9726C432-92C1-489C-9623-55330B3530E8");

        explicit VKImage(VKDevice& dev);
        VKImage(VKDevice& dev, const ImageDesc& desc);
        ~VKImage() override;

        const ImageDesc& GetDesc() override;
        Rc<IImageView> CreateView(ImageAspectFlags aspectFlags) override;

        void AllocateMemory(MemoryType type) override;
        void BindMemory(const DeviceMemorySlice& memory) override;
    };
} // namespace FE::Osmium
