#pragma once
#include <FeGPU/Fence/IFence.h>

namespace FE::GPU
{
    enum class CommandListClass
    {
        Graphics,
        Compute,
        Copy
    };

    class IDevice
    {
    public:
        virtual ~IDevice() = default;
        virtual RefCountPtr<IFence> CreateFence(uint64_t value) = 0;
    };
}
