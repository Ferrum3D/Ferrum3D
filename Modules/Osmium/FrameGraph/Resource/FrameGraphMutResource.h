#pragma once
#include <FeCore/RTTI/RTTI.h>

namespace FE::FG
{
    class FrameGraphMutResource
    {
        friend class FrameGraphBuilder;

        UInt64 m_ID;

        explicit FrameGraphMutResource(UInt64 id);

    public:
        FE_CLASS_RTTI(FrameGraphMutResource, "5F7D9F25-9D38-404B-AA9C-5DE2B38EDDEE");

        [[nodiscard]] UInt64 GetID() const;
    };

    inline FrameGraphMutResource::FrameGraphMutResource(UInt64 id)
        : m_ID(id)
    {
    }

    inline UInt64 FrameGraphMutResource::GetID() const
    {
        return m_ID;
    }
}
