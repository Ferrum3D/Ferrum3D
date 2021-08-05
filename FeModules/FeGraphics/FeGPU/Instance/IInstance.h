#pragma once
#include <FeCore/Memory/Memory.h>
#include <FeGPU/Adapter/IAdapter.h>

namespace FE::GPU
{
    enum class GraphicsAPI
    {
        None,
        Vulkan
    };

    struct InstanceDesc
    {
    };

    class IInstance
    {
    public:
        virtual ~IInstance() = default;

        virtual Vector<RefCountPtr<IAdapter>>& GetAdapters() = 0;
    };

    RefCountPtr<IInstance> CreateGraphicsAPIInstance(InstanceDesc desc, GraphicsAPI api);
} // namespace FE::GPU
