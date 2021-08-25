#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeGPU/Adapter/IAdapter.h>
#include <FeGPU/Common/BaseTypes.h>

namespace FE::GPU
{
    struct InstanceDesc
    {
        FE_STRUCT_RTTI(InstanceDesc, "20125FD5-EFCB-426E-B1EE-50DF51457171");
    };

    class IInstance : public IObject
    {
    public:
        virtual ~IInstance() = default;

        FE_CLASS_RTTI(IInstance, "C6CC0410-BB89-484A-8FD7-9DF99AE3CD31");

        virtual Vector<RefCountPtr<IAdapter>>& GetAdapters() = 0;
    };

    RefCountPtr<IInstance> CreateGraphicsAPIInstance(InstanceDesc desc, GraphicsAPI api);
} // namespace FE::GPU
