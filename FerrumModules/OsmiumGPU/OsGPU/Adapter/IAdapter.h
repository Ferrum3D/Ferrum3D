#pragma once
#include <FeCore/Strings/String.h>

namespace FE::Osmium
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
        const char* Name;
        AdapterType Type;
    };

    class IInstance;
    class IDevice;

    class IAdapter : public Memory::RefCountedObjectBase
    {
    public:
        FE_CLASS_RTTI(IAdapter, "860B2CCD-3918-4943-8D49-33040D76EA0D");

        ~IAdapter() override                   = default;
        virtual IInstance& GetInstance()       = 0;
        virtual AdapterDesc& GetDesc()         = 0;
        virtual Rc<IDevice> CreateDevice() = 0;
    };
} // namespace FE::Osmium
