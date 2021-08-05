#pragma once
#include <FeGPU/Common/VKConfig.h>
#include <FeGPU/Image/IImage.h>

namespace FE::GPU
{
    class VKDevice;

    class VKImage : public IImage
    {
        VKDevice* m_Device;

    public:
        vk::UniqueImage UniqueImage;
        vk::Image Image;
        ImageDesc Desc;

        VKImage(VKDevice& dev);

        virtual const ImageDesc& GetDesc() override;

        virtual void* Map() override;
        virtual void Unmap() override;
        virtual void BindMemory(const RefCountPtr<IDeviceMemory>& memory, uint64_t offset) override;
    };
} // namespace FE::GPU
