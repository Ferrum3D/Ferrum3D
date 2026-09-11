#pragma once
#include <Core/IO/Assets.h>
#include <Core/Jobs/Base.h>

namespace FE::IO
{
    enum class AssetFinalizeResult : uint8_t
    {
        kSucceeded,
        kPending,
        kFailed,
    };


    //! @brief Type-specific publication/finalization hook for decoded asset candidates.
    //!
    //! Hooks execute from AssetManager::Tick on the main thread. Pending work is polled on later ticks, and failure prevents
    //! publication of the candidate.
    struct Streamer
    {
        virtual ~Streamer() = default;

        //! @brief Start or perform type-specific finalization for a deserialized candidate.
        virtual AssetFinalizeResult FinalizeAssetLoading(AssetSlot& assetSlot, void* candidate) = 0;

        //! @brief Poll whether previously started asynchronous finalization has completed.
        virtual AssetFinalizeResult PollFinalize(AssetSlot& assetSlot, void* candidate) = 0;

        //! @brief True when finalization needs dependencies to be published before it can start.
        virtual bool RequiresFinalizedDependencies() const
        {
            return false;
        }
    };


    //! @brief No-op finalizer used by asset types that need no specialized publication work.
    struct DefaultStreamer final : public Streamer
    {
        AssetFinalizeResult FinalizeAssetLoading(AssetSlot& assetSlot, void* candidate) override;
        AssetFinalizeResult PollFinalize(AssetSlot& assetSlot, void* candidate) override;
    };


    //! @brief Process-wide coordinator for stable slots, shared load operations, and residency acquisitions.
    //!
    //! LoadAsset is thread-safe and starts asynchronous wavefront metadata discovery. Per-asset operations are coalesced globally,
    //! while every returned AssetRequest owns an independent deduplicated residency contribution for its hard closure. Tick is
    //! advances type finalization and opens publication gates for usable groups.
    struct AssetManager final
    {
        //! @brief Initialize process-wide manager state before issuing requests.
        static void Init();

        //! @brief Wait for dispatched discovery jobs and destroy manager-owned operation and slot storage.
        //!
        //! The current milestone requires every AssetRequest to be released before shutdown.
        static void Shutdown();

        //! @brief Acquire a root asset and asynchronously load its transitive hard-dependency closure.
        [[nodiscard]] static AssetRequest LoadAsset(AssetID assetId);

        //! @brief Find the stable slot reserved for an asset, or null if the asset has never participated in a request.
        [[nodiscard]] static AssetSlot* FindAssetSlot(AssetID assetId);

        //! @brief Register a non-owning type-specific finalizer. Registration must remain valid until Shutdown.
        static void RegisterStreamer(Rtti::TypeID typeId, Streamer* streamer);

        //! @brief Typed-link convenience overload; the link's runtime handle does not contribute residency.
        template<class T, DependencyKind TKind = DependencyKind::kHard>
        [[nodiscard]] static AssetRequest LoadAsset(Link<T, TKind> link)
        {
            return LoadAsset(link.GetAssetID());
        }

        //! @brief Execute main-thread finalization and publication work.
        static void Tick();

#if FE_DEVELOPMENT
        //! @brief Install deterministic discovery barriers for tests; both groups are retained until replaced or cleared.
        //!
        //! A metadata job signals entered immediately before waiting on resume. Null groups disable the corresponding action.
        static void SetDiscoveryBarrierForTests(WaitGroup* entered, WaitGroup* resume);

        //! @brief Return how many physical metadata reads were started for an asset in the current manager lifetime.
        [[nodiscard]] static uint32_t GetMetadataReadCountForTests(AssetID assetId);
#endif

    private:
        friend struct AssetRequest;

        //! Increment the public-copy count of an opaque acquisition record.
        static void AddRequestRef(Internal::AssetAcquisition* acquisition);

        //! Release a public copy and, on the final copy, its recorded residency membership.
        static void ReleaseRequest(Internal::AssetAcquisition* acquisition);

        //! Transition a pending acquisition to canceled without canceling work shared by other acquisitions.
        static void CancelRequest(Internal::AssetAcquisition* acquisition);

        struct Impl;
        static Impl* GImpl;
    };
} // namespace FE::IO
