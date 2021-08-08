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

        VKImage(VKDevice& dev);

        virtual const ImageDesc& GetDesc() override;
    };
} // namespace FE::GPU
