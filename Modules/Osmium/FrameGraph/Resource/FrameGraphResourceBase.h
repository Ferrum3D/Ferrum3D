#pragma once
#include <FrameGraph/Resource/IFrameGraphResource.h>
#include <FeCore/Strings/String.h>

namespace FE::FG
{
    class FrameGraphResourceBase : public Object<IFrameGraphResource>
    {
    protected:
        String m_Name;

    public:
        FE_CLASS_RTTI(FrameGraphResourceBase, "11CBFFBA-4AC5-46BD-94AE-F613B045616E");

        USize Index = 0;

        [[nodiscard]] const char* GetName() const;
    };

    const char* FrameGraphResourceBase::GetName() const
    {
        return m_Name.Data();
    }
} // namespace FE::FG
