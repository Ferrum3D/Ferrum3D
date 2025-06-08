#pragma once
#include <FeCore/Memory/Memory.h>

namespace FE::Graphics::Core
{
    struct DeviceObject;
    struct Resource;


    struct Device : public Memory::RefCountedObjectBase
    {
        FE_RTTI_Class(Device, "23D426E6-3322-4CB2-9800-DEBA7C3DEAC0");

        virtual void WaitIdle() = 0;
        virtual void EndFrame() = 0;

    private:
        friend DeviceObject;
        friend Resource;

        virtual void QueueObjectDispose(DeviceObject* object) = 0;
        virtual uint32_t RegisterResource(Resource* resource) = 0;
        virtual void UnregisterResource(uint32_t id, Resource* resource) = 0;
    };
} // namespace FE::Graphics::Core
