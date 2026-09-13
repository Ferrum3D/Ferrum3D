#pragma once
#include <Core/IO/BaseIO.h>
#include <Core/Serialization/Serialization.h>

namespace FE::IO
{
    enum class DependencyKind : uint32_t;

    template<class T>
    struct AssetLease;

    namespace Internal
    {
        //! @brief Opaque shared state owned by one logical residency acquisition.
        struct AssetAcquisition;

        //! @brief Find the stable slot for a logical asset without adding residency.
        AssetSlot* FindAssetSlot(AssetID assetId);

        //! @brief Try to pin the generation currently published in slot.
        const void* AcquireAssetGeneration(AssetSlot* slot, void*& generationToken);

        //! @brief Release a generation pin acquired by AcquireAssetGeneration.
        void ReleaseAssetGeneration(void* generationToken);
    } // namespace Internal


    //! @brief Short-lived pin on one concrete published generation.
    //!
    //! Retirement closes admission before detaching a generation from its slot. Existing reads remain valid until their final
    //! pin is released; a read attempted after admission closes returns an empty object. Generation reads must be released before
    //! AssetManager shutdown, while ordinary non-owning AssetHandle objects may outlive it.
    template<class T>
    struct AssetRead final
    {
        AssetRead() = default;
        ~AssetRead()
        {
            Reset();
        }

        AssetRead(const AssetRead&) = delete;
        AssetRead& operator=(const AssetRead&) = delete;

        AssetRead(AssetRead&& other) noexcept
            : m_instance(other.m_instance)
            , m_generationToken(other.m_generationToken)
        {
            other.m_instance = nullptr;
            other.m_generationToken = nullptr;
        }

        AssetRead& operator=(AssetRead&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                m_instance = other.m_instance;
                m_generationToken = other.m_generationToken;
                other.m_instance = nullptr;
                other.m_generationToken = nullptr;
            }
            return *this;
        }

        [[nodiscard]] explicit operator bool() const
        {
            return m_instance != nullptr;
        }

        [[nodiscard]] const T* Get() const
        {
            return m_instance;
        }

        [[nodiscard]] const T* operator->() const
        {
            FE_AssertDebug(m_instance);
            return m_instance;
        }

        void Reset()
        {
            if (m_generationToken)
                Internal::ReleaseAssetGeneration(m_generationToken);
            m_instance = nullptr;
            m_generationToken = nullptr;
        }

    private:
        template<class U>
        friend struct AssetHandle;
        template<class U>
        friend struct AssetLease;

        explicit AssetRead(AssetSlot* slot)
        {
            const void* instance = Internal::AcquireAssetGeneration(slot, m_generationToken);
            m_instance = static_cast<const T*>(instance);
        }

        const T* m_instance = nullptr;
        void* m_generationToken = nullptr;
    };


    //! @brief Terminal state of an AssetRequest.
    enum class AssetLoadResult : uint8_t
    {
        //! Required metadata, payload, finalization, or publication work remains.
        kPending,
        //! The complete hard closure was published and its residency contribution remains held.
        kSucceeded,
        //! Required metadata failed or a typed dependency was incompatible; residency was rolled back.
        kFailed,
        //! This acquisition was canceled independently of any shared operation; residency was rolled back.
        kCanceled,
    };


    //! @brief Shared visibility gate for a publication group.
    //!
    //! Every slot in a hard-reference cycle points at the same gate. Candidate pointers are installed first and the gate is
    //! opened with release semantics only after the complete group is usable.
    struct AssetPublicationGate final
    {
        std::atomic<bool> m_isOpen = false;
    };

    namespace Internal
    {
        //! @brief Non-templated access shared by typed asset handles and residency tickets.
        //!
        //! The slot address is stable for the lifetime of AssetManager. The instance and generation stored in the slot may change
        //! when publication, replacement, and retirement are implemented.
        struct AssetHandleImpl
        {
            //! @brief True when this handle identifies a stable asset slot.
            [[nodiscard]] bool IsValid() const
            {
                return m_slot != nullptr;
            }

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
                {
                    return *this;
                }

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
                {
                    return;
                }

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

        //! Published object pointer. Readers also validate m_publicationGate before accessing it.
        std::atomic<void*> m_instance = nullptr;

        //! Publication readiness flag. Discovery completion alone never sets this flag.
        std::atomic<bool> m_completed = false;

        //! Publication gate shared by every member of the current reference group.
        std::atomic<AssetPublicationGate*> m_publicationGate = nullptr;

        //! Monotonically increasing published-generation number.
        std::atomic<uint32_t> m_generation = 0;

        //! Number of non-owning handles that currently point at this stable slot.
        std::atomic<uint32_t> m_weakRefCount = 0;

        //! Aggregate residency demand, including one contribution per acquisition containing this slot.
        std::atomic<uint32_t> m_strongRefCount = 0;

        //! One manager-owner reference plus one reference for each public weak or single-slot residency handle.
        std::atomic<uint32_t> m_lifetimeRefCount = 1;
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

        //! @brief True after the acquisition publishes, fails, or is canceled.
        [[nodiscard]] bool IsCompleted() const;

        //! @brief True when this acquisition was canceled before publication completed.
        [[nodiscard]] bool IsCanceled() const;

        //! @brief Get this acquisition's final result; an invalid request reports kFailed.
        [[nodiscard]] AssetLoadResult GetResult() const;

        //! @brief Get the stable slot for the requested root asset, or null for an invalid request.
        [[nodiscard]] AssetSlot* GetAssetSlot() const;

        //! @brief Cancel this acquisition and release its recorded residency contribution.
        void Cancel();

        //! @brief Suspend a background fiber until publication reaches a terminal result.
        void Wait() const;

        //! @brief Suspend until the dependency metadata closure is known, without requiring Tick.
        void WaitForDiscovery() const;

        //! @brief Return the dependency-discovery result independently of publication.
        [[nodiscard]] AssetLoadResult GetDiscoveryResult() const;

        //! @brief Return a stable diagnostic after a failed request, or an empty view otherwise.
        [[nodiscard]] festd::string_view GetError() const;

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

        //! @brief Pin the concrete generation currently visible through this lease.
        [[nodiscard]] AssetRead<T> Read() const
        {
            return AssetRead<T>(GetAssetSlot());
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

        //! @brief Pin the concrete generation currently visible through this handle.
        [[nodiscard]] AssetRead<T> Read() const
        {
            return AssetRead<T>(GetAssetSlot());
        }
    };


    //! @brief Serializable logical asset reference whose dependency behavior is encoded by TKind.
    //!
    //! The link stores identity only. Resolving it produces a temporary non-owning handle to the manager-owned stable slot.
    template<class T, DependencyKind TKind = DependencyKind::kHard>
    struct Link final
    {
        //! Dependency semantics available to serialization and loading code at compile time.
        static constexpr DependencyKind kKind = TKind;

        //! @brief Get the persistent logical asset identity.
        [[nodiscard]] AssetID GetAssetID() const
        {
            return m_id;
        }

#if FE_DEVELOPMENT
        //! @brief Change the logical target.
        void SetAssetID(const AssetID id)
        {
            m_id = id;
        }
#endif

        //! @brief Resolve the current non-owning runtime handle without initiating a load.
        [[nodiscard]] AssetHandle<T> GetAssetHandle() const
        {
            return AssetHandle<T>(Internal::FindAssetSlot(m_id));
        }

    private:
        friend struct Serialization::Serializer<Link<T, TKind>>;

        //! Serialized logical identity; never contains a physical path or artifact location.
        AssetID m_id = AssetID::kNull;
    };
} // namespace FE::IO


namespace FE::Serialization
{
    //! Asset links serialize only their logical ID. Runtime lookup is deferred until GetAssetHandle is called.
    template<class T, IO::DependencyKind TKind>
    struct Serializer<IO::Link<T, TKind>>
    {
        static ResultCode Serialize(SerializationContext& context, const IO::Link<T, TKind>& value)
        {
            return SerializeValue(context, value.m_id);
        }

        static ResultCode Deserialize(DeserializationContext& context, IO::Link<T, TKind>& value)
        {
            return DeserializeValue(context, value.m_id);
        }

        static uint64_t GetSchemaHash()
        {
            return Serialization::GetSchemaHash<IO::AssetID>();
        }

        static uint32_t GetVersion()
        {
            return 0;
        }
    };
} // namespace FE::Serialization
