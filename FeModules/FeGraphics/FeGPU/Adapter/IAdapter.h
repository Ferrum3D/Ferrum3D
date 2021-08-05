#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Strings/String.h>
#include <FeGPU/Device/IDevice.h>

namespace FE::GPU
{
    enum class AdapterType
    {
        None,
        Integrated,
        Discrete,
        Virtual,
        CPU
    };

    struct AdapterDesc
    {
        String Name;
        AdapterType Type;
    };

    class IInstance;

    class IAdapter
    {
    public:
        virtual ~IAdapter()                         = default;
        virtual IInstance& GetInstance()            = 0;
        virtual AdapterDesc& GetDesc()              = 0;
        virtual RefCountPtr<IDevice> CreateDevice() = 0;
    };
} // namespace FE::GPU
