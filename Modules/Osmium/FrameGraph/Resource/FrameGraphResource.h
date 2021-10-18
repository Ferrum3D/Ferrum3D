#pragma once
#include <FrameGraph/Resource/FrameGraphMutResource.h>

namespace FE::FG
{
    class FrameGraphResource
    {
        friend class FrameGraphBuilder;

        UInt64 m_ID;

        explicit FrameGraphResource(UInt64 id);

    public:
        FE_CLASS_RTTI(FrameGraphResource, "5E524F1D-6424-4645-9DA6-E4F99D5532C1");

        FrameGraphResource(const FrameGraphMutResource& resource); // NOLINT

        [[nodiscard]] UInt64 GetID() const;
    };

    inline FrameGraphResource::FrameGraphResource(UInt64 id)
        : m_ID(id)
    {
    }

    inline FrameGraphResource::FrameGraphResource(const FrameGraphMutResource& resource)
        : m_ID(resource.GetID())
    {
    }

    inline UInt64 FrameGraphResource::GetID() const
    {
        return m_ID;
    }
}
