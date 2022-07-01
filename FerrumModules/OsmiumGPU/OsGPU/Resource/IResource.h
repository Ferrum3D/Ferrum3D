#pragma once
#include <FeCore/Memory/Object.h>
#include <OsGPU/Resource/ResourceState.h>

namespace FE::Osmium
{
    class IResource : public IObject
    {
        friend class VKCommandBuffer;

    protected:
        virtual void SetCurrentState(ResourceState state) = 0;

    public:
        [[nodiscard]] virtual ResourceState GetCurrentState() const noexcept = 0;
    };
} // namespace FE::Osmium
