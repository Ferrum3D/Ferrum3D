#pragma once
#include <FeGPU/Common/VKConfig.h>
#include <FeGPU/Image/IImage.h>

namespace FE::GPU
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

        VKImage(VKDevice& dev);

        virtual const ImageDesc& GetDesc() override;
    };
} // namespace FE::GPU
