#pragma once
#include <OsGPU/Common/VKConfig.h>
#include <OsGPU/Image/IImage.h>

namespace FE::Osmium
{
    class VKDevice;

    class VKImage : public Object<IImage>
    {
        VKDevice* m_Device;

    public:
        vk::UniqueImage UniqueImage;
        vk::Image Image;
        ImageDesc Desc;

        FE_CLASS_RTTI(VKImage, "9726C432-92C1-489C-9623-55330B3530E8");

        explicit VKImage(VKDevice& dev);
        VKImage(VKDevice& dev, const ImageDesc& desc);

        const ImageDesc& GetDesc() override;
        Shared<IImageView> CreateRenderTargetView() override;
    };
} // namespace FE::Osmium
