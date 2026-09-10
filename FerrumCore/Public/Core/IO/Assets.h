#pragma once
#include <Core/IO/BaseIO.h>

namespace FE::IO
{
    template<class T>
    struct AssetLease;

    namespace Internal
    {
        //! @brief Opaque shared state owned by one logical residency acquisition.
        struct AssetAcquisition;
    } // namespace Internal


    //! @brief Terminal state of dependency discovery for an AssetRequest.
    //!
    //! This state is independent of AssetSlot readiness. A successful request has discovered and retained its hard dependency
    //! closure, but the corresponding slots do not become ready until a later publication stage installs usable instances.
    enum class AssetLoadResult : uint8_t
    {
        //! At least one required metadata operation has not completed.
        kPending,
        //! The complete hard closure was validated and its residency contribution remains held.
        kSucceeded,
        //! Required metadata failed or a typed dependency was incompatible; residency was rolled back.
        kFailed,
        //! This acquisition was canceled independently of any shared operation; residency was rolled back.
        kCanceled,
    };

    namespace Internal
    {
        //! @brief Non-templated access shared by typed asset handles and residency tickets.
        //!
        //! The slot address is stable for the lifetime of AssetManager. The instance and generation stored in the slot may change
        //! when publication, replacement, and retirement are implemented.
        struct AssetHandleImpl
        {
            //! @brief Get the currently published generation number.
            [[nodiscard]] uint32_t GetGeneration() const;

            //! @brief True when the slot contains a fully published instance usable by readers.
            [[nodiscard]] bool IsReady() const;

            //! @brief Get the stable slot referenced by this handle, or null for an invalid handle.
            [[nodiscard]] AssetSlot* GetAssetSlot() const
            {
                return m_slot;
            }

        protected:
            //! @brief Acquire the currently published instance pointer without changing residency.
            const void* GetAssetInstance() const;

            //! Stable, manager-owned slot. Derived handle kinds decide which slot reference counter they affect.
            AssetSlot* m_slot = nullptr;
        };


        //! @brief CRTP implementation of copy, move, reset, and reference bookkeeping for slot handles.
        //!
        //! Derived classes provide InternalAddRef and InternalRelease to select weak or strong slot bookkeeping. Adopt and Detach
        //! transfer an already-accounted reference and therefore deliberately do not increment or decrement it.
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

            //! @brief Take ownership of an existing reference without incrementing its counter.
            void Adopt(AssetSlot* slot)
            {
                static_cast<THandle*>(this)->InternalRelease();
                m_slot = slot;
            }

            //! @brief Relinquish ownership of the current reference without decrementing its counter.
            [[nodiscard]] AssetSlot* Detach()
            {
                AssetSlot* slot = m_slot;
                m_slot = nullptr;
                return slot;
            }

            //! @brief Release the current reference and make the handle invalid.
            void Invalidate()
            {
                static_cast<THandle*>(this)->InternalRelease();
                FE_AssertDebug(m_slot == nullptr);
            }
        };
    } // namespace Internal


    //! @brief Stable indirection point for one logical asset.
    //!
    //! Handles refer to slots rather than concrete instances so publication or replacement can change the generation without
    //! invalidating every handle. Slot storage is manager-owned and is not recycled by the current implementation.
    struct AssetSlot final
    {
        //! Logical identity represented by this slot.
        AssetID m_assetId;

        //! Runtime type decoded from pinned artifact metadata. Null until metadata discovery succeeds.
        Rtti::TypeID m_typeId;

        //! Artifact selected for the current operation/generation. Null until metadata discovery succeeds.
        ArtifactID m_currentArtifactId;

        //! Published object pointer. Stage 3 leaves this null; publication updates it with release semantics.
        std::atomic<void*> m_instance = nullptr;

        //! Publication readiness flag. Discovery completion alone never sets this flag.
        std::atomic<bool> m_completed = false;

        //! Monotonically increasing published-generation number.
        std::atomic<uint32_t> m_generation = 0;

        //! Strongly connected hard-reference group used for later group binding/publication, or kInvalidIndex when acyclic.
        std::atomic<uint32_t> m_referenceGroup = kInvalidIndex;

        //! Number of non-owning handles that currently point at this stable slot.
        std::atomic<uint32_t> m_weakRefCount = 0;

        //! Aggregate residency demand, including one contribution per acquisition containing this slot.
        std::atomic<uint32_t> m_strongRefCount = 0;

        //! Reserved controller for generation payload work; metadata-discovery status belongs to AssetRequest instead.
        Rc<Async::IController> m_asyncController;
    };


    //! @brief Shared handle for one root acquisition and its deduplicated transitive hard-dependency set.
    //!
    //! Copies share the same acquisition record and residency contribution. The final Reset/destruction releases the recorded
    //! membership exactly once. Cancel affects that acquisition only; shared per-asset metadata operations may continue for other
    //! acquisitions. Requests must not outlive AssetManager.
    struct AssetRequest final
    {
        AssetRequest() = default;
        ~AssetRequest();

        AssetRequest(const AssetRequest& other);
        AssetRequest(AssetRequest&& other) noexcept;
        AssetRequest& operator=(const AssetRequest& other);
        AssetRequest& operator=(AssetRequest&& other) noexcept;

        //! @brief True when this object identifies an acquisition record.
        [[nodiscard]] bool IsValid() const;

        //! @brief True after discovery succeeds, fails, or is canceled.
        [[nodiscard]] bool IsCompleted() const;

        //! @brief True when this acquisition was canceled before discovery completed.
        [[nodiscard]] bool IsCanceled() const;

        //! @brief Get this acquisition's discovery result; an invalid request reports kFailed.
        [[nodiscard]] AssetLoadResult GetResult() const;

        //! @brief Get the stable slot for the requested root asset, or null for an invalid request.
        [[nodiscard]] AssetSlot* GetAssetSlot() const;

        //! @brief Cancel this acquisition and release its recorded residency contribution.
        void Cancel();

        //! @brief Suspend the calling fiber until discovery reaches a terminal result.
        void Wait() const;

        //! @brief Release this request copy; the final copy releases the shared acquisition.
        void Reset();

    private:
        friend struct AssetManager;

        explicit AssetRequest(Internal::AssetAcquisition* acquisition);

        //! Shared, manager-coordinated state. Its work references may keep it alive after the final public copy is released.
        Internal::AssetAcquisition* m_acquisition = nullptr;
    };


    //! @brief Loading semantics attached to a serialized asset dependency.
    enum class DependencyKind : uint32_t
    {
        //! Automatically discover and retain the dependency with the requesting acquisition.
        kHard,
        //! Record a reference without automatically loading or retaining the target.
        kSoft,
        //! Reserved for an explicitly optional loading policy; currently treated as non-hard.
        kOptional,
    };


    //! @brief Non-owning reference to a stable asset slot.
    //!
    //! Weak tickets keep slot bookkeeping accurate but do not contribute residency demand.
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


    //! @brief Owning reference that contributes one unit of residency demand to a single slot.
    //!
    //! Graph acquisitions use AssetRequest and a recorded membership set. This lower-level ticket remains useful for a single
    //! already-known slot and is the base of AssetLease.
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


    //! @brief Typed owning view of the currently published instance in one slot.
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

        //! @brief Get the published instance, or null when no generation is currently installed.
        [[nodiscard]] const T* Get() const
        {
            return static_cast<const T*>(GetAssetInstance());
        }
    };


    //! @brief Typed non-owning view of the currently published instance in one stable slot.
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

        //! @brief Get the published instance, or null when no generation is currently installed.
        [[nodiscard]] const T* Get() const
        {
            return static_cast<const T*>(GetAssetInstance());
        }
    };


    //! @brief Serializable logical asset reference whose dependency behavior is encoded by TKind.
    //!
    //! Only m_id is persistent data. m_handle is a runtime binding cache populated during object binding and never owns residency.
    template<class T, DependencyKind TKind = DependencyKind::kHard>
    struct Link final
    {
        //! Dependency semantics available to serialization and binding code at compile time.
        static constexpr DependencyKind kKind = TKind;

        //! @brief Get the persistent logical asset identity.
        [[nodiscard]] AssetID GetAssetID() const
        {
            return m_id;
        }

#if FE_DEVELOPMENT
        //! @brief Change the logical target and discard any runtime binding to the old slot.
        void SetAssetID(const AssetID id)
        {
            m_id = id;
            m_handle.Invalidate();
        }
#endif

        //! @brief Get a copy of the current non-owning runtime binding.
        [[nodiscard]] AssetHandle<T> GetAssetHandle() const
        {
            return m_handle;
        }

    private:
        //! Serialized logical identity; never contains a physical path or artifact location.
        AssetID m_id = AssetID::kNull;

        //! Transient stable-slot binding established after metadata validation and deserialization.
        AssetHandle<T> m_handle;
    };
} // namespace FE::IO
