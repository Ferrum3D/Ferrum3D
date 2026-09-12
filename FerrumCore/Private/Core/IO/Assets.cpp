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
            if (!m_slot->m_completed.load(std::memory_order_acquire))
            {
                return false;
            }

            AssetPublicationGate* gate = m_slot->m_publicationGate.load(std::memory_order_acquire);
            return !gate || gate->m_isOpen.load(std::memory_order_acquire);
        }


        const void* AssetHandleImpl::GetAssetInstance() const
        {
            FE_AssertDebug(m_slot);
            if (!IsReady())
            {
                return nullptr;
            }
            return m_slot->m_instance.load(std::memory_order_acquire);
        }
    } // namespace Internal


    void WeakResidencyTicket::InternalAddRef()
    {
        if (m_slot)
        {
            ++m_slot->m_weakRefCount;
            ++m_slot->m_lifetimeRefCount;
        }
    }


    void WeakResidencyTicket::InternalRelease()
    {
        if (m_slot)
        {
            AssetSlot* slot = m_slot;
            --slot->m_weakRefCount;
            m_slot = nullptr;
            const uint32_t previous = slot->m_lifetimeRefCount.fetch_sub(1, std::memory_order_acq_rel);
            FE_Assert(previous > 0, "Asset slot lifetime count underflow");
            if (previous == 1)
                Memory::DefaultDelete(slot);
        }
    }


    void ResidencyTicket::InternalAddRef()
    {
        if (m_slot)
        {
            ++m_slot->m_strongRefCount;
            ++m_slot->m_lifetimeRefCount;
        }
    }


    void ResidencyTicket::InternalRelease()
    {
        if (m_slot)
        {
            AssetSlot* slot = m_slot;
            --slot->m_strongRefCount;
            m_slot = nullptr;
            const uint32_t previous = slot->m_lifetimeRefCount.fetch_sub(1, std::memory_order_acq_rel);
            FE_Assert(previous > 0, "Asset slot lifetime count underflow");
            if (previous == 1)
                Memory::DefaultDelete(slot);
        }
    }
} // namespace FE::IO
