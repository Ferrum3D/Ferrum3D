#pragma once
#include <FeCore/Memory/Memory.h>
#include <GPU/Device/IDevice.h>

namespace FE::FG
{
    //! \brief Holds a virtual FrameGraph resource, can be realized.
    class IFrameGraphResource : public IObject
    {
    public:
        FE_CLASS_RTTI(IFrameGraphResource, "E09EB536-833A-4165-AFF1-689D8405F99F");

        ~IFrameGraphResource() override = default;

        //! \brief Make virtual resource real
        virtual void Realize(GPU::IDevice* device) = 0;

        //! \brief Dispose real resource.
        virtual void Dispose() = 0;
    };
} // namespace FE::FG
