#pragma once
#include <Core/IO/Assets.h>
#include <Core/Jobs/Base.h>

namespace FE::IO
{
    //! @brief Type-specific publication/finalization hook for decoded asset candidates.
    //!
    //! Stage 3 only performs metadata discovery, so these hooks are reserved for the publication pipeline implemented by the next
    //! stage. Implementations must not treat metadata discovery alone as a ready asset.
    struct Streamer
    {
        virtual ~Streamer() = default;

        //! @brief Start or perform type-specific finalization; true means finalization completed synchronously.
        virtual bool FinalizeAssetLoading(AssetSlot& assetSlot) = 0;

        //! @brief Poll whether previously started asynchronous finalization has completed.
        virtual bool IsFinalizeCompleted(AssetSlot& assetSlot) = 0;
    };


    //! @brief No-op finalizer used by asset types that need no specialized publication work.
    struct DefaultStreamer final : public Streamer
    {
        bool FinalizeAssetLoading(AssetSlot& assetSlot) override;
        bool IsFinalizeCompleted(AssetSlot& assetSlot) override;
    };


    //! @brief Process-wide coordinator for stable slots, shared load operations, and residency acquisitions.
    //!
    //! LoadAsset is thread-safe and starts asynchronous wavefront metadata discovery. Per-asset operations are coalesced globally,
    //! while every returned AssetRequest owns an independent deduplicated residency contribution for its hard closure. Tick is
    //! reserved for main-thread publication and currently performs no work.
    struct AssetManager final
    {
        //! @brief Initialize process-wide manager state before issuing requests.
        static void Init();

        //! @brief Wait for dispatched discovery jobs and destroy manager-owned operation and slot storage.
        //!
        //! The current milestone requires every AssetRequest to be released before shutdown.
        static void Shutdown();

        //! @brief Acquire a root asset and asynchronously discover its transitive hard-dependency closure.
        [[nodiscard]] static AssetRequest LoadAsset(AssetID assetId);

        //! @brief Find the stable slot reserved for an asset, or null if the asset has never participated in a request.
        [[nodiscard]] static AssetSlot* FindAssetSlot(AssetID assetId);

        //! @brief Typed-link convenience overload; the link's runtime handle does not contribute residency.
        template<class T, DependencyKind TKind = DependencyKind::kHard>
        [[nodiscard]] static AssetRequest LoadAsset(Link<T, TKind> link)
        {
            return LoadAsset(link.GetAssetID());
        }

        //! @brief Execute main-thread publication and retirement work.
        //!
        //! Stage 3 performs discovery entirely in background jobs, so this function is intentionally empty until stage 4.
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
