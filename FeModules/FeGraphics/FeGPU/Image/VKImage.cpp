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
}
