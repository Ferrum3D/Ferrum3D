#pragma once
#include <Core/IO/BaseIO.h>

namespace FE::IO
{
    template<class T>
    struct AssetLease;

    namespace Internal
    {
        struct AssetHandleImpl
        {
            [[nodiscard]] uint32_t GetGeneration() const;
            [[nodiscard]] bool IsReady() const;
            [[nodiscard]] Async::Status GetStatus() const;

            [[nodiscard]] AssetSlot* GetAssetSlot() const
            {
                return m_slot;
            }

        protected:
            const void* GetAssetInstance() const;

            AssetSlot* m_slot = nullptr;
        };


        template<class THandle>
        struct AssetHandleBase : public AssetHandleImpl
        {
            AssetHandleBase() = default;
            ~AssetHandleBase()
            {
                static_cast<THandle*>(this)->InternalRelease();
            }

            explicit AssetHandleBase(AssetSlot* slot)
            {
                m_slot = slot;
                static_cast<THandle*>(this)->InternalAddRef();
            }

            AssetHandleBase(const AssetHandleBase& other)
            {
                m_slot = other.GetAssetSlot();
                static_cast<THandle*>(this)->InternalAddRef();
            }

            AssetHandleBase(AssetHandleBase&& other) noexcept
            {
                m_slot = other.Detach();
            }

            AssetHandleBase& operator=(const AssetHandleBase& other)
            {
                if (this == &other)
                    return *this;

                Reset(other.GetAssetSlot());
                return *this;
            }

            AssetHandleBase& operator=(AssetHandleBase&& other) noexcept
            {
                Adopt(other.Detach());
                return *this;
            }

            void Reset(AssetSlot* slot)
            {
                if (m_slot == slot)
                    return;

                static_cast<THandle*>(this)->InternalRelease();
                m_slot = slot;
                static_cast<THandle*>(this)->InternalAddRef();
            }

            // Takes ownership of an existing reference without incrementing it.
            void Adopt(AssetSlot* slot)
            {
                static_cast<THandle*>(this)->InternalRelease();
                m_slot = slot;
            }

            [[nodiscard]] AssetSlot* Detach()
            {
                AssetSlot* slot = m_slot;
                m_slot = nullptr;
                return slot;
            }

            void Invalidate()
            {
                static_cast<THandle*>(this)->InternalRelease();
                FE_AssertDebug(m_slot == nullptr);
            }
        };
    } // namespace Internal


    struct AssetSlot final
    {
        AssetID m_assetId;
        Rtti::TypeID m_typeId;
        ArtifactID m_currentArtifactId;
        std::atomic<void*> m_instance = nullptr;
        std::atomic<bool> m_completed = false;
        std::atomic<uint32_t> m_generation = 0;

        std::atomic<uint32_t> m_weakRefCount = 0;
        std::atomic<uint32_t> m_strongRefCount = 0;
        Rc<Async::IController> m_asyncController;
    };


    enum class DependencyKind : uint32_t
    {
        kHard,
        kSoft,
        kOptional,
    };


    struct WeakResidencyTicket : public Internal::AssetHandleBase<WeakResidencyTicket>
    {
        WeakResidencyTicket() = default;
        explicit WeakResidencyTicket(AssetSlot* slot)
            : AssetHandleBase(slot)
        {
        }

    protected:
        friend struct Internal::AssetHandleBase<WeakResidencyTicket>;

        void InternalAddRef();
        void InternalRelease();
    };


    struct ResidencyTicket : public Internal::AssetHandleBase<ResidencyTicket>
    {
        ResidencyTicket() = default;
        explicit ResidencyTicket(AssetSlot* slot)
            : AssetHandleBase(slot)
        {
        }

    protected:
        friend struct Internal::AssetHandleBase<ResidencyTicket>;

        void InternalAddRef();
        void InternalRelease();
    };


    template<class T>
    struct AssetLease final : public ResidencyTicket
    {
        AssetLease() = default;
        explicit AssetLease(AssetSlot* slot)
            : ResidencyTicket(slot)
        {
        }

        explicit AssetLease(const ResidencyTicket& ticket)
            : ResidencyTicket(ticket)
        {
        }

        [[nodiscard]] const T* Get() const
        {
            return static_cast<const T*>(GetAssetInstance());
        }
    };


    template<class T>
    struct AssetHandle final : public WeakResidencyTicket
    {
        AssetHandle() = default;
        explicit AssetHandle(AssetSlot* slot)
            : WeakResidencyTicket(slot)
        {
        }

        explicit AssetHandle(const AssetLease<T>& lease)
            : WeakResidencyTicket(lease.GetAssetSlot())
        {
        }

        [[nodiscard]] const T* Get() const
        {
            return static_cast<const T*>(GetAssetInstance());
        }
    };


    template<class T, DependencyKind TKind = DependencyKind::kHard>
    struct Link final
    {
        static constexpr DependencyKind kKind = TKind;

        [[nodiscard]] AssetID GetAssetID() const
        {
            return m_id;
        }

#if FE_DEVELOPMENT
        void SetAssetID(const AssetID id)
        {
            m_id = id;
            m_handle.Invalidate();
        }
#endif

        [[nodiscard]] AssetHandle<T> GetAssetHandle() const
        {
            return m_handle;
        }

    private:
        AssetID m_id = AssetID::kNull;
        AssetHandle<T> m_handle;
    };
} // namespace FE::IO
