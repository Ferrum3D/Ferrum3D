#pragma once
#include <FeCore/RTTI/RTTI.h>
#include <FeCore/Strings/FixedString.h>
#include <FeCore/Strings/String.h>
#include <HAL/Common/BaseTypes.h>
#include <HAL/DeviceMemorySlice.h>
#include <HAL/DeviceObject.h>
#include <HAL/MemoryType.h>

namespace FE::Graphics::HAL
{
    class Resource
        : public DeviceObject
        , public festd::intrusive_list_node
    {
    protected:
        FixStr64 m_Name;

        inline void Register()
        {
            std::lock_guard lk{ m_pDevice->m_ResourceListLock };
            m_pDevice->m_ResourceList.push_back(*this);
        }

        inline ~Resource()
        {
            festd::intrusive_list<>::remove(*this);
        }

    public:
        [[nodiscard]] inline StringSlice GetName() const
        {
            return m_Name;
        }

        virtual void AllocateMemory(MemoryType type) = 0;
        virtual void BindMemory(const DeviceMemorySlice& memory) = 0;
    };
} // namespace FE::Graphics::HAL
