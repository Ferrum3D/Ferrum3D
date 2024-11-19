#pragma once
#include <FeCore/RTTI/RTTI.h>
#include <FeCore/Strings/FixedString.h>
#include <FeCore/Strings/String.h>
#include <Graphics/RHI/Common/BaseTypes.h>
#include <Graphics/RHI/DeviceMemorySlice.h>
#include <Graphics/RHI/DeviceObject.h>

namespace FE::Graphics::RHI
{
    struct Resource
        : public DeviceObject
        , public festd::intrusive_list_node
    {
        [[nodiscard]] StringSlice GetName() const
        {
            return m_name;
        }

        virtual void AllocateMemory(MemoryType type) = 0;
        virtual void BindMemory(const DeviceMemorySlice& memory) = 0;

    protected:
        FixedString<126> m_name;

        void Register()
        {
            std::lock_guard lk{ m_device->m_resourceListLock };
            m_device->m_resourceList.push_back(*this);
        }

        ~Resource()
        {
            festd::intrusive_list<>::remove(*this);
        }
    };
} // namespace FE::Graphics::RHI
