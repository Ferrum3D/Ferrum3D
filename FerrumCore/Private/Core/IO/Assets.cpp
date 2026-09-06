#include <Core/IO/Assets.h>
#include <Core/IO/Async.h>

namespace FE::IO
{
    namespace Internal
    {
        uint32_t AssetHandleImpl::GetGeneration() const
        {
            FE_AssertDebug(m_slot);
            return m_slot->m_generation.load(std::memory_order_acquire);
        }


        bool AssetHandleImpl::IsReady() const
        {
            FE_AssertDebug(m_slot);
            return m_slot->m_completed.load(std::memory_order_acquire);
        }


        Async::Status AssetHandleImpl::GetStatus() const
        {
            FE_AssertDebug(m_slot);
            return m_slot->m_asyncController->GetStatus();
        }


        const void* AssetHandleImpl::GetAssetInstance() const
        {
            FE_AssertDebug(m_slot);
            return m_slot->m_instance.load(std::memory_order_acquire);
        }
    } // namespace Internal


    void WeakResidencyTicket::InternalAddRef()
    {
        if (m_slot)
            ++m_slot->m_weakRefCount;
    }


    void WeakResidencyTicket::InternalRelease()
    {
        if (m_slot)
        {
            --m_slot->m_weakRefCount;
            m_slot = nullptr;
        }
    }


    void ResidencyTicket::InternalAddRef()
    {
        if (m_slot)
            ++m_slot->m_strongRefCount;
    }


    void ResidencyTicket::InternalRelease()
    {
        if (m_slot)
        {
            --m_slot->m_strongRefCount;
            m_slot = nullptr;
        }
    }
} // namespace FE::IO
