#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/IO/Async.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Threading/Mutex.h>
#include <festd/unordered_map.h>
#include <festd/vector.h>

namespace FE::IO
{
    namespace Internal
    {
        //! Shared state for one logical LoadAsset call and all copies of the returned AssetRequest.
        //!
        //! Fields other than the atomic public result/reference count are protected by AssetManager::Impl::m_mutex. Every
        //! subscribed load operation contributes one work reference, allowing the record to survive cancellation and final public
        //! release until all callbacks have detached.
        struct AssetAcquisition final
        {
            //! Number of AssetRequest copies visible to callers.
            std::atomic<uint32_t> m_externalRefCount = 1;

            //! Number of pending operation subscriber entries that still point to this record.
            uint32_t m_workRefCount = 0;

            //! Logical root identity retained for future diagnostics and dependency-path reporting.
            AssetID m_rootId = AssetID::kNull;

            //! Stable slot for the requested root, exposed by AssetRequest::GetAssetSlot.
            AssetSlot* m_rootSlot = nullptr;

            //! Deduplicated acquired closure mapped to each asset's required runtime type.
            festd::unordered_dense_map<AssetID, Rtti::TypeID> m_membership;

            //! Exact slots whose residency counters were incremented by this acquisition.
            festd::vector<AssetSlot*> m_residentSlots;

            //! Number of membership operations whose metadata has not reached a terminal state.
            uint32_t m_pendingMetadata = 0;

            //! Prevents cancellation, failure, and final release from decrementing membership more than once.
            bool m_residencyReleased = false;

            //! Publicly observable discovery state; release publishes all preceding metadata/membership writes.
            std::atomic<AssetLoadResult> m_result = AssetLoadResult::kPending;

            //! Fiber-aware completion primitive signaled exactly once when m_result leaves kPending.
            Rc<WaitGroup> m_completion = WaitGroup::Create();
        };
    } // namespace Internal


    namespace
    {
        //! Metadata discovery state of one shared per-asset load operation.
        enum class OperationState : uint8_t
        {
            //! Metadata job is dispatched or awaiting completion.
            kLoading,
            //! Normalized ArtifactRecord is pinned in the operation.
            kSucceeded,
            //! Structured ArtifactMetadataError is pinned in the operation.
            kFailed,
        };


        //! Shared and pinned metadata operation for one logical asset.
        //!
        //! An operation is inserted into the registry before its job is dispatched. It remains manager-owned after completion so
        //! late acquisitions can reuse normalized metadata without another read and so dependency discovery sees a consistent
        //! graph snapshot.
        struct LoadOperation final
        {
            //! Logical identity and registry key.
            AssetID m_assetId = AssetID::kNull;

            //! Stable slot reserved before asynchronous work begins.
            AssetSlot* m_slot = nullptr;

            //! State protected by the manager mutex.
            OperationState m_state = OperationState::kLoading;

            //! Pinned normalized metadata, valid only after kSucceeded.
            ArtifactRecord m_artifactRecord;

            //! Pinned structured failure, valid only after kFailed.
            ArtifactMetadataError m_error;

            //! Acquisitions awaiting this operation; each entry owns one acquisition work reference.
            festd::vector<Internal::AssetAcquisition*> m_subscribers;

            //! Diagnostic count used to assert coalescing; currently always one for a created operation.
            uint32_t m_metadataReadCount = 0;
        };


        //! Build a structured metadata I/O error before ArtifactStore decoding is possible.
        ArtifactMetadataError MakeError(const AssetID assetId, const Path& source, const festd::string_view message)
        {
            ArtifactMetadataError error;
            error.m_code = ArtifactMetadataErrorCode::kIoError;
            error.m_assetId = assetId;
            error.m_source = source;
            error.m_message = message;
            return error;
        }
    } // namespace


    //! Synchronized process-wide implementation of AssetManager.
    //!
    //! The mutex protects both registries, all operation state, and non-atomic acquisition state. It is never held across job
    //! dispatch, file I/O, metadata decoding, or fiber waits.
    struct AssetManager::Impl final
    {
        //! Registry/acquisition synchronization boundary.
        Threading::Mutex m_mutex;

        //! Type-specific finalizers reserved for stage-4 publication.
        festd::unordered_dense_map<Rtti::TypeID, Streamer*> m_assetTypeToStreamerMap;

        //! Fallback finalizer reserved for asset types without a registered streamer.
        DefaultStreamer m_defaultStreamer;

        //! Stable slot lookup by logical identity.
        festd::unordered_dense_map<AssetID, AssetSlot*> m_slots;

        //! Shared, pinned load-operation lookup by logical identity.
        festd::unordered_dense_map<AssetID, LoadOperation*> m_operations;

        //! Owning list for individually allocated slots; indirection keeps slot addresses stable across growth.
        festd::vector<AssetSlot*> m_ownedSlots;

        //! Owning list for individually allocated operations retained through shutdown.
        festd::vector<LoadOperation*> m_ownedOperations;

        //! Detached job completions retained so shutdown can wait before destroying operation storage.
        festd::vector<Rc<WaitGroup>> m_operationJobs;

        //! Rejects new public root requests once shutdown begins.
        bool m_isShuttingDown = false;

        //! Allocator for manager-lifetime strongly connected reference-group identities.
        uint32_t m_nextReferenceGroup = 0;

#if FE_DEVELOPMENT
        //! Optional signal emitted when a test-controlled metadata job reaches its barrier.
        Rc<WaitGroup> m_discoveryEntered;

        //! Optional test-controlled gate awaited before resolving and reading metadata.
        Rc<WaitGroup> m_discoveryResume;
#endif

        ~Impl();

        //! Find or reserve stable slot storage. Requires m_mutex.
        AssetSlot& FindOrCreateSlot(AssetID assetId);

        //! Find or register a shared operation and append newly created work for out-of-lock dispatch. Requires m_mutex.
        LoadOperation& FindOrCreateOperation(AssetID assetId, festd::vector<LoadOperation*>& operationsToStart);

        //! Add one asset to an acquisition's deduplicated closure and subscribe to its metadata if necessary. Requires m_mutex.
        void Join(Internal::AssetAcquisition& acquisition, AssetID assetId, Rtti::TypeID expectedType,
                  festd::vector<LoadOperation*>& operationsToStart);

        //! Validate one successful member and traverse only its direct hard dependencies. Requires m_mutex.
        void Expand(Internal::AssetAcquisition& acquisition, LoadOperation& operation,
                    festd::vector<LoadOperation*>& operationsToStart);

        //! Commit decoded metadata, advance subscribers, and collect the next discovery wave.
        void CompleteOperation(LoadOperation& operation, ArtifactDecodeResult&& decodeResult);

        //! Dispatch a collected discovery wave with no registry lock held.
        void StartOperations(festd::span<LoadOperation*> operations);

        //! Dispatch asynchronous read/decode work for one pre-registered operation.
        void StartOperation(LoadOperation& operation);

        //! Remove this acquisition's exact residency contributions at most once. Requires m_mutex.
        void ReleaseResidency(Internal::AssetAcquisition& acquisition);

        //! Publish a terminal request result and signal waiters exactly once. Requires m_mutex.
        void Finish(Internal::AssetAcquisition& acquisition, AssetLoadResult result);

        //! Find hard-reference strongly connected components for later group binding/publication. Requires m_mutex.
        void DetectReferenceGroups(Internal::AssetAcquisition& acquisition);

        //! Destroy an acquisition after both public and operation work references reach zero. Requires m_mutex.
        void TryDeleteAcquisition(Internal::AssetAcquisition* acquisition);
    };

    AssetManager::Impl* AssetManager::GImpl = nullptr;


    AssetManager::Impl::~Impl()
    {
        for (LoadOperation* operation : m_ownedOperations)
            Memory::DefaultDelete(operation);
        for (AssetSlot* slot : m_ownedSlots)
            Memory::DefaultDelete(slot);
    }


    AssetSlot& AssetManager::Impl::FindOrCreateSlot(const AssetID assetId)
    {
        // Slot reservation: publish one stable address before any asynchronous work can refer to the asset.
        const auto existing = m_slots.find(assetId);
        if (existing != m_slots.end())
            return *existing->second;

        AssetSlot* slot = Memory::DefaultNew<AssetSlot>();
        slot->m_assetId = assetId;
        m_ownedSlots.push_back(slot);
        m_slots.insert({ assetId, slot });
        return *slot;
    }


    LoadOperation& AssetManager::Impl::FindOrCreateOperation(const AssetID assetId,
                                                             festd::vector<LoadOperation*>& operationsToStart)
    {
        // Operation coalescing: insert first, dispatch later, so concurrent roots and cycles always find shared state.
        const auto existing = m_operations.find(assetId);
        if (existing != m_operations.end())
            return *existing->second;

        LoadOperation* operation = Memory::DefaultNew<LoadOperation>();
        operation->m_assetId = assetId;
        operation->m_slot = &FindOrCreateSlot(assetId);
        operation->m_metadataReadCount = 1;
        m_ownedOperations.push_back(operation);
        m_operations.insert({ assetId, operation });
        operationsToStart.push_back(operation);
        return *operation;
    }


    void AssetManager::Impl::Join(Internal::AssetAcquisition& acquisition, const AssetID assetId, const Rtti::TypeID expectedType,
                                  festd::vector<LoadOperation*>& operationsToStart)
    {
        FE_PROFILER_ZONE();

        // Membership stage: each acquisition contributes residency only on its first visit to an asset.
        if (acquisition.m_result.load(std::memory_order_relaxed) != AssetLoadResult::kPending)
            return;

        auto member = acquisition.m_membership.find(assetId);
        if (member != acquisition.m_membership.end())
        {
            if (!expectedType.IsValid())
                return;

            if (member->second.IsValid() && member->second != expectedType)
            {
                Finish(acquisition, AssetLoadResult::kFailed);
                return;
            }

            // A root initially has no expected type. A cycle may later reach it through a typed edge, which must still be enforced.
            member->second = expectedType;

            const auto operation = m_operations.find(assetId);
            FE_Assert(operation != m_operations.end());

            const bool metadataAvailable = operation->second->m_state == OperationState::kSucceeded;
            if (metadataAvailable && operation->second->m_artifactRecord.m_assetTypeId != expectedType)
                Finish(acquisition, AssetLoadResult::kFailed);
            return;
        }

        LoadOperation& operation = FindOrCreateOperation(assetId, operationsToStart);
        acquisition.m_membership.insert({ assetId, expectedType });
        acquisition.m_residentSlots.push_back(operation.m_slot);
        operation.m_slot->m_strongRefCount.fetch_add(1, std::memory_order_relaxed);

        if (operation.m_state == OperationState::kFailed)
        {
            Finish(acquisition, AssetLoadResult::kFailed);
            return;
        }

        if (operation.m_state == OperationState::kSucceeded)
        {
            // Late join: replay pinned normalized metadata synchronously and enqueue only genuinely new dependency operations.
            Expand(acquisition, operation, operationsToStart);
            return;
        }

        // In-flight join: keep the acquisition alive until this shared operation consumes its subscriber entry.
        ++acquisition.m_pendingMetadata;
        ++acquisition.m_workRefCount;
        operation.m_subscribers.push_back(&acquisition);
    }


    void AssetManager::Impl::Expand(Internal::AssetAcquisition& acquisition, LoadOperation& operation,
                                    festd::vector<LoadOperation*>& operationsToStart)
    {
        FE_PROFILER_ZONE();

        // Type validation precedes traversal so a bad hard edge cannot begin unrelated dependency work.
        const Rtti::TypeID expectedType = acquisition.m_membership.find(operation.m_assetId)->second;
        if (expectedType.IsValid() && expectedType != operation.m_artifactRecord.m_assetTypeId)
        {
            Finish(acquisition, AssetLoadResult::kFailed);
            return;
        }

        // Wavefront expansion: soft/optional links remain representable but do not contribute automatic residency.
        for (const ArtifactDependencyRecord& dependency : operation.m_artifactRecord.m_dependencies)
        {
            if (dependency.m_kind == DependencyKind::kHard)
                Join(acquisition, dependency.m_assetId, dependency.m_expectedTypeId, operationsToStart);
        }
    }


    void AssetManager::Impl::ReleaseResidency(Internal::AssetAcquisition& acquisition)
    {
        // Rollback/release stage: use recorded membership, never a traversal of metadata that may change in a later generation.
        if (acquisition.m_residencyReleased)
            return;

        acquisition.m_residencyReleased = true;
        for (AssetSlot* slot : acquisition.m_residentSlots)
        {
            const uint32_t previousCount = slot->m_strongRefCount.fetch_sub(1, std::memory_order_relaxed);
            FE_Assert(previousCount > 0, "Asset residency count underflow");
        }
    }


    void AssetManager::Impl::Finish(Internal::AssetAcquisition& acquisition, const AssetLoadResult result)
    {
        FE_PROFILER_ZONE();

        // Request completion is separate from slot publication readiness.
        if (acquisition.m_result.load(std::memory_order_relaxed) != AssetLoadResult::kPending)
            return;

        if (result == AssetLoadResult::kSucceeded)
            DetectReferenceGroups(acquisition);
        else
            ReleaseResidency(acquisition);

        acquisition.m_result.store(result, std::memory_order_release);
        acquisition.m_completion->Signal();
    }


    void AssetManager::Impl::DetectReferenceGroups(Internal::AssetAcquisition& acquisition)
    {
        FE_PROFILER_ZONE();

        //! Per-operation scratch state for Tarjan strongly connected component discovery.
        struct Node final
        {
            //! Pinned operation whose hard edges are traversed.
            LoadOperation* m_operation = nullptr;
            //! DFS discovery order, or kInvalidIndex before the node is visited.
            uint32_t m_index = kInvalidIndex;
            //! Lowest discovery index reachable through the active DFS stack.
            uint32_t m_lowLink = kInvalidIndex;
            //! True while this node is eligible to join the current strongly connected component.
            bool m_onStack = false;
        };

        // Reference-group stage: build an acquisition-local view over the already complete pinned hard-dependency graph.
        festd::vector<Node> nodes;
        festd::unordered_dense_map<AssetID, uint32_t> nodeById;
        nodes.reserve(static_cast<uint32_t>(acquisition.m_membership.size()));
        for (const auto& [assetId, expectedType] : acquisition.m_membership)
        {
            FE_Unused(expectedType);
            const auto operation = m_operations.find(assetId);
            FE_Assert(operation != m_operations.end());
            nodeById.insert({ assetId, nodes.size() });
            nodes.push_back({ operation->second });
        }

        // Tarjan traversal identifies maximal groups that must later bind and publish coherently.
        festd::vector<uint32_t> stack;
        uint32_t nextIndex = 0;
        const auto visit = [&](auto&& self, const uint32_t nodeIndex) -> void {
            Node& node = nodes[nodeIndex];
            node.m_index = nextIndex;
            node.m_lowLink = nextIndex;
            ++nextIndex;
            stack.push_back(nodeIndex);
            node.m_onStack = true;

            for (const ArtifactDependencyRecord& dependency : node.m_operation->m_artifactRecord.m_dependencies)
            {
                if (dependency.m_kind != DependencyKind::kHard)
                    continue;

                const auto targetEntry = nodeById.find(dependency.m_assetId);
                if (targetEntry == nodeById.end())
                    continue;

                Node& target = nodes[targetEntry->second];
                if (target.m_index == kInvalidIndex)
                {
                    self(self, targetEntry->second);
                    node.m_lowLink = Math::Min(node.m_lowLink, target.m_lowLink);
                }
                else if (target.m_onStack)
                {
                    node.m_lowLink = Math::Min(node.m_lowLink, target.m_index);
                }
            }

            if (node.m_lowLink != node.m_index)
                return;

            festd::inline_vector<uint32_t, 8> component;
            for (;;)
            {
                const uint32_t memberIndex = stack.back();
                stack.pop_back();
                nodes[memberIndex].m_onStack = false;
                component.push_back(memberIndex);
                if (memberIndex == nodeIndex)
                    break;
            }

            bool isReferenceGroup = component.size() > 1;
            if (!isReferenceGroup)
            {
                for (const ArtifactDependencyRecord& dependency : node.m_operation->m_artifactRecord.m_dependencies)
                {
                    if (dependency.m_kind == DependencyKind::kHard && dependency.m_assetId == node.m_operation->m_assetId)
                        isReferenceGroup = true;
                }
            }

            if (!isReferenceGroup)
                return;

            // Group identity belongs to stable slots, not acquisitions. Reuse an existing assignment on late acquisitions.
            uint32_t group = kInvalidIndex;
            for (const uint32_t memberIndex : component)
            {
                const uint32_t existingGroup =
                    nodes[memberIndex].m_operation->m_slot->m_referenceGroup.load(std::memory_order_acquire);
                if (existingGroup == kInvalidIndex)
                    continue;

                FE_Assert(group == kInvalidIndex || group == existingGroup, "Inconsistent asset reference group");
                group = existingGroup;
            }

            if (group == kInvalidIndex)
                group = m_nextReferenceGroup++;

            for (const uint32_t memberIndex : component)
                nodes[memberIndex].m_operation->m_slot->m_referenceGroup.store(group, std::memory_order_release);
        };

        for (uint32_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
        {
            if (nodes[nodeIndex].m_index == kInvalidIndex)
                visit(visit, nodeIndex);
        }
    }


    void AssetManager::Impl::CompleteOperation(LoadOperation& operation, ArtifactDecodeResult&& decodeResult)
    {
        FE_PROFILER_ZONE();

        // Completion/reconciliation stage: commit immutable operation state and advance every interested acquisition atomically.
        festd::vector<LoadOperation*> operationsToStart;
        {
            std::lock_guard lock{ m_mutex };
            if (decodeResult)
            {
                operation.m_artifactRecord = std::move(decodeResult.value());
                operation.m_slot->m_typeId = operation.m_artifactRecord.m_assetTypeId;
                operation.m_slot->m_currentArtifactId = operation.m_artifactRecord.m_artifactId;
                operation.m_state = OperationState::kSucceeded;
            }
            else
            {
                operation.m_error = std::move(decodeResult.error());
                operation.m_state = OperationState::kFailed;
            }

            for (Internal::AssetAcquisition* acquisition : operation.m_subscribers)
            {
                // Consume this subscription before expansion; newly discovered operations add their own pending/work references.
                FE_Assert(acquisition->m_pendingMetadata > 0);
                --acquisition->m_pendingMetadata;

                if (acquisition->m_result.load(std::memory_order_relaxed) == AssetLoadResult::kPending)
                {
                    if (operation.m_state == OperationState::kSucceeded)
                        Expand(*acquisition, operation, operationsToStart);
                    else
                        Finish(*acquisition, AssetLoadResult::kFailed);

                    const bool hasNoPendingMetadata = acquisition->m_pendingMetadata == 0;
                    const bool isStillPending =
                        acquisition->m_result.load(std::memory_order_relaxed) == AssetLoadResult::kPending;
                    if (hasNoPendingMetadata && isStillPending)
                        Finish(*acquisition, AssetLoadResult::kSucceeded);
                }

                FE_Assert(acquisition->m_workRefCount > 0);
                --acquisition->m_workRefCount;
                TryDeleteAcquisition(acquisition);
            }
            operation.m_subscribers.clear();
        }

        // Dispatch the next wave only after releasing the registry lock. Jobs may complete immediately and re-enter reconciliation.
        StartOperations(operationsToStart);
    }


    void AssetManager::Impl::StartOperation(LoadOperation& operation)
    {
        FE_PROFILER_ZONE();

        // Background metadata stage: resolve, read to end, decode, validate, and normalize without requiring AssetManager::Tick.
        Rc<WaitGroup> job = WaitGroup::Create();
        Jobs::DispatchBackground(
            [this, &operation] {
                FE_PROFILER_ZONE_NAMED("ReadAndDecodeMetadata");

#if FE_DEVELOPMENT
                // Test-only scheduling barrier makes concurrent and late-join timing deterministic without sleeps.
                Rc<WaitGroup> entered;
                Rc<WaitGroup> resume;
                {
                    std::lock_guard lock{ m_mutex };
                    entered = m_discoveryEntered;
                    resume = m_discoveryResume;
                }
                if (entered)
                    entered->Signal();
                if (resume)
                    resume->Wait();
#endif

                const ResolvedDataSource source = ArtifactStore::ResolveMeta(operation.m_assetId);
                if (!source.IsValid())
                {
                    CompleteOperation(
                        operation,
                        festd::unexpected(
                            MakeError(operation.m_assetId, source.m_filePath, "metadata source could not be resolved")));
                    return;
                }

                festd::pmr::vector<std::byte> bytes;
                Rc<WaitGroup> completion = WaitGroup::Create();
                Async::Batch batch(source, completion.Get());
                batch.ReadAppendToEnd(bytes);

                // The fiber yields while I/O is active; no manager lock is held across submission or waiting.
                const Rc<Async::IController> controller = Async::Read(batch);
                completion->Wait();
                if (controller->GetStatus() != Async::Status::kSucceeded)
                {
                    CompleteOperation(
                        operation,
                        festd::unexpected(MakeError(operation.m_assetId, source.m_filePath, "metadata read failed")));
                    return;
                }

                ArtifactResolutionContext context;
                context.m_assetId = operation.m_assetId;
                context.m_metadataSource = source;

                // ArtifactStore owns the JSON contract and physical-source normalization; the manager only consumes its result.
                CompleteOperation(operation, ArtifactStore::Decode(bytes, context));
            },
            job.Get());

        std::lock_guard lock{ m_mutex };
        m_operationJobs.push_back(std::move(job));
    }


    void AssetManager::Impl::StartOperations(const festd::span<LoadOperation*> operations)
    {
        FE_PROFILER_ZONE();

        // A collected wave contains only operations newly inserted under the registry lock.
        for (LoadOperation* operation : operations)
            StartOperation(*operation);
    }


    void AssetManager::Impl::TryDeleteAcquisition(Internal::AssetAcquisition* acquisition)
    {
        FE_PROFILER_ZONE();

        // Public lifetime and callback lifetime are independent; both must end before reclaiming the record.
        const bool hasExternalRefs = acquisition->m_externalRefCount.load(std::memory_order_acquire) != 0;
        if (!hasExternalRefs && acquisition->m_workRefCount == 0)
            Memory::DefaultDelete(acquisition);
    }


    bool DefaultStreamer::FinalizeAssetLoading(AssetSlot& assetSlot)
    {
        FE_Unused(assetSlot);
        return true;
    }


    bool DefaultStreamer::IsFinalizeCompleted(AssetSlot& assetSlot)
    {
        FE_Unused(assetSlot);
        return true;
    }


    void AssetManager::Init()
    {
        FE_PROFILER_ZONE();

        FE_Assert(GImpl == nullptr, "Asset Manager already initialized");
        GImpl = Memory::DefaultNew<Impl>();
    }


    void AssetManager::Shutdown()
    {
        FE_PROFILER_ZONE();

        FE_Assert(GImpl != nullptr, "Asset Manager not initialized");
        Impl* impl = GImpl;
        {
            std::lock_guard lock{ impl->m_mutex };
            impl->m_isShuttingDown = true;
        }

        // Detached jobs capture operation/manager storage, so all recorded jobs must finish before either is destroyed.
        WaitGroup::WaitAll(impl->m_operationJobs);
        Memory::DefaultDelete(impl);
        GImpl = nullptr;
    }


    AssetRequest AssetManager::LoadAsset(const AssetID assetId)
    {
        FE_PROFILER_ZONE();

        FE_Assert(GImpl != nullptr, "Asset Manager not initialized");
        if (!assetId.IsValid())
            return {};

        // Root acquisition stage: allocate independent ownership/status state for this caller.
        Internal::AssetAcquisition* acquisition = Memory::DefaultNew<Internal::AssetAcquisition>();
        acquisition->m_rootId = assetId;
        festd::vector<LoadOperation*> operationsToStart;
        {
            // Registry stage: reserve the root slot, join/create its operation, and publish all bookkeeping before dispatch.
            std::lock_guard lock{ GImpl->m_mutex };
            FE_Assert(!GImpl->m_isShuttingDown, "Cannot load assets during shutdown");
            GImpl->Join(*acquisition, assetId, Rtti::TypeID::kNull, operationsToStart);
            acquisition->m_rootSlot = &GImpl->FindOrCreateSlot(assetId);
            const bool hasNoPendingMetadata = acquisition->m_pendingMetadata == 0;
            const bool isStillPending = acquisition->m_result.load(std::memory_order_relaxed) == AssetLoadResult::kPending;
            if (hasNoPendingMetadata && isStillPending)
                GImpl->Finish(*acquisition, AssetLoadResult::kSucceeded);
        }

        // Dispatch stage: no registry lock is held while jobs, I/O, or user-visible wait primitives can run.
        GImpl->StartOperations(operationsToStart);
        return AssetRequest(acquisition);
    }


    AssetSlot* AssetManager::FindAssetSlot(const AssetID assetId)
    {
        if (GImpl == nullptr)
            return nullptr;

        std::lock_guard lock{ GImpl->m_mutex };
        const auto slot = GImpl->m_slots.find(assetId);
        return slot == GImpl->m_slots.end() ? nullptr : slot->second;
    }


    void AssetManager::Tick()
    {
        // Stage 3 completes discovery in the background. Publication starts in stage 4.
    }


    void AssetManager::AddRequestRef(Internal::AssetAcquisition* acquisition)
    {
        acquisition->m_externalRefCount.fetch_add(1, std::memory_order_relaxed);
    }


    void AssetManager::ReleaseRequest(Internal::AssetAcquisition* acquisition)
    {
        FE_PROFILER_ZONE();

        const uint32_t previousCount = acquisition->m_externalRefCount.fetch_sub(1, std::memory_order_acq_rel);
        FE_Assert(previousCount > 0, "Asset request reference count underflow");
        if (previousCount != 1)
            return;

        FE_Assert(GImpl != nullptr, "Asset request outlived AssetManager");
        std::lock_guard lock{ GImpl->m_mutex };
        if (acquisition->m_result.load(std::memory_order_relaxed) == AssetLoadResult::kPending)
            GImpl->Finish(*acquisition, AssetLoadResult::kCanceled);
        else
            GImpl->ReleaseResidency(*acquisition);

        GImpl->TryDeleteAcquisition(acquisition);
    }


    void AssetManager::CancelRequest(Internal::AssetAcquisition* acquisition)
    {
        FE_PROFILER_ZONE();

        FE_Assert(GImpl != nullptr, "Asset Manager not initialized");
        std::lock_guard lock{ GImpl->m_mutex };
        GImpl->Finish(*acquisition, AssetLoadResult::kCanceled);
    }


#if FE_DEVELOPMENT
    void AssetManager::SetDiscoveryBarrierForTests(WaitGroup* entered, WaitGroup* resume)
    {
        FE_Assert(GImpl != nullptr, "Asset Manager not initialized");
        std::lock_guard lock{ GImpl->m_mutex };
        GImpl->m_discoveryEntered = entered;
        GImpl->m_discoveryResume = resume;
    }


    uint32_t AssetManager::GetMetadataReadCountForTests(const AssetID assetId)
    {
        FE_Assert(GImpl != nullptr, "Asset Manager not initialized");
        std::lock_guard lock{ GImpl->m_mutex };
        const auto operation = GImpl->m_operations.find(assetId);
        return operation == GImpl->m_operations.end() ? 0 : operation->second->m_metadataReadCount;
    }
#endif


    AssetRequest::AssetRequest(Internal::AssetAcquisition* acquisition)
        : m_acquisition(acquisition)
    {
    }


    AssetRequest::~AssetRequest()
    {
        Reset();
    }


    AssetRequest::AssetRequest(const AssetRequest& other)
        : m_acquisition(other.m_acquisition)
    {
        if (m_acquisition)
            AssetManager::AddRequestRef(m_acquisition);
    }


    AssetRequest::AssetRequest(AssetRequest&& other) noexcept
        : m_acquisition(other.m_acquisition)
    {
        other.m_acquisition = nullptr;
    }


    AssetRequest& AssetRequest::operator=(const AssetRequest& other)
    {
        if (this == &other)
            return *this;
        Reset();
        m_acquisition = other.m_acquisition;
        if (m_acquisition)
            AssetManager::AddRequestRef(m_acquisition);
        return *this;
    }


    AssetRequest& AssetRequest::operator=(AssetRequest&& other) noexcept
    {
        if (this == &other)
            return *this;
        Reset();
        m_acquisition = other.m_acquisition;
        other.m_acquisition = nullptr;
        return *this;
    }


    bool AssetRequest::IsValid() const
    {
        return m_acquisition != nullptr;
    }


    bool AssetRequest::IsCompleted() const
    {
        return m_acquisition && m_acquisition->m_result.load(std::memory_order_acquire) != AssetLoadResult::kPending;
    }


    bool AssetRequest::IsCanceled() const
    {
        return GetResult() == AssetLoadResult::kCanceled;
    }


    AssetLoadResult AssetRequest::GetResult() const
    {
        return m_acquisition ? m_acquisition->m_result.load(std::memory_order_acquire) : AssetLoadResult::kFailed;
    }


    AssetSlot* AssetRequest::GetAssetSlot() const
    {
        return m_acquisition ? m_acquisition->m_rootSlot : nullptr;
    }


    void AssetRequest::Cancel()
    {
        if (m_acquisition)
            AssetManager::CancelRequest(m_acquisition);
    }


    void AssetRequest::Wait() const
    {
        if (m_acquisition && !IsCompleted())
            m_acquisition->m_completion->Wait();
    }


    void AssetRequest::Reset()
    {
        if (!m_acquisition)
            return;

        AssetManager::ReleaseRequest(m_acquisition);
        m_acquisition = nullptr;
    }
} // namespace FE::IO
