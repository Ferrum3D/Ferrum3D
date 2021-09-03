#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeCore/Memory/Object.h>
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
        FE_STRUCT_RTTI(AdapterDesc, "11A8F0B5-48A0-4F1A-9023-8B3F65F2ECE1");
        String Name;
        AdapterType Type;
    };

    class IInstance;

    class IAdapter : public IObject
    {
    public:
        FE_CLASS_RTTI(IAdapter, "860B2CCD-3918-4943-8D49-33040D76EA0D");

        ~IAdapter() override                        = default;
        virtual IInstance& GetInstance()            = 0;
        virtual AdapterDesc& GetDesc()              = 0;
        virtual Shared<IDevice> CreateDevice() = 0;
    };
} // namespace FE::GPU
