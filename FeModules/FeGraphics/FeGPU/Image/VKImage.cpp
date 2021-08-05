#include <FeGPU/Image/VKImage.h>

namespace FE::GPU
{
    VKImage::VKImage(VKDevice& dev)
        : m_Device(&dev)
    {
    }

    const ImageDesc& VKImage::GetDesc()
    {
        return Desc;
    }

    void* VKImage::Map()
    {
        return nullptr;
    }

    void VKImage::Unmap()
    {
    }

    void VKImage::BindMemory(const RefCountPtr<IDeviceMemory>& memory, uint64_t offset)
    {
    }
}
