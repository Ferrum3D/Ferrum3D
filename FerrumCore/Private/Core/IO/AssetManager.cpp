#include <Core/Containers/ConcurrentQueue.h>
#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/IO/Async.h>
#include <Core/IO/StreamBase.h>
#include <Core/Jobs/Jobs.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Serialization/JsonSerialization.h>
#include <Core/Threading/Mutex.h>
#include <Core/Threading/Thread.h>
#include <festd/unordered_map.h>
#include <festd/vector.h>

namespace FE::IO
{
    namespace
    {
        struct Operation;
    }

    namespace Internal
    {
        struct AssetAcquisition final
        {
            std::atomic<uint32_t> m_externalRefs = 1;
            uint32_t m_workRefs = 0;
            AssetSlot* m_root = nullptr;
            festd::unordered_dense_map<AssetID, Rtti::TypeID> m_members;
            festd::unordered_dense_map<AssetID, AssetID> m_parents;
            festd::vector<AssetSlot*> m_residentSlots;
            festd::vector<Operation*> m_memberOperations;
            uint32_t m_pendingMetadata = 0;
            bool m_released = false;
            std::atomic<AssetLoadResult> m_discovery = AssetLoadResult::kPending;
            std::atomic<AssetLoadResult> m_result = AssetLoadResult::kPending;
            Rc<WaitGroup> m_discoveryEvent = WaitGroup::Create();
            Rc<WaitGroup> m_completionEvent = WaitGroup::Create();
            festd::inline_string m_error;
            void* m_targetOperation = nullptr;
            std::atomic<bool> m_managerAlive = true;
        };
    } // namespace Internal

    namespace
    {
        enum class State : uint8_t
        {
            // Metadata discovery is in flight. The dependency graph cannot be considered complete yet.
            kMetadata,
            // Metadata is valid and the primary payload is being read and decoded on a background worker.
            kPayload,
            // A deserialized CPU object is waiting for main-thread finalization.
            kCandidate,
            // The type streamer has started asynchronous finalization and must be polled by Tick.
            kFinalizing,
            // The object is visible through its slot and may be used by readers.
            kPublished,
            // Admission is closed and the detached object is waiting for concrete-generation readers to drain.
            kRetiring,
            // Retirement completed. A later request creates a fresh operation for this logical asset.
            kDormant,
            // A terminal error occurred before publication. No candidate from this operation is exposed.
            kFailed
        };


        struct ReferenceGroup final
        {
            festd::vector<Operation*> m_members;
            AssetPublicationGate* m_gate = nullptr;
        };


        struct Operation final : ConcurrentOnceConsumedQueue::Node
        {
            AssetID m_id;
            AssetSlot* m_slot = nullptr;
            State m_state = State::kMetadata;
            ArtifactRecord m_record;
            festd::inline_string m_error;
            festd::vector<Internal::AssetAcquisition*> m_subscribers;
            const Rtti::Type* m_type = nullptr;
            void* m_candidate = nullptr;
            void* m_retiredObject = nullptr;
            AssetFinalizeResult m_finalize = AssetFinalizeResult::kPending;
            std::atomic<uint32_t> m_readHolds = 0;
            bool m_cancelRequested = false;
            uint32_t m_interestCount = 0;
            uint64_t m_serial = 0;
            Rc<Async::IController> m_activeController;
            ReferenceGroup* m_publicationGroup = nullptr;
            ReferenceGroup* m_retirementGroup = nullptr;
            festd::vector<Operation*> m_dependencyHolds;
        };


        void DestroyObject(Operation& operation, void* object)
        {
            FE_PROFILER_ZONE();

            if (!object)
                return;

            if (operation.m_type->m_destructor)
                operation.m_type->m_destructor(object);

            Memory::DefaultFree(object);
        }


        ArtifactMetadataError IoError(AssetID id, const Path& source, festd::string_view message)
        {
            ArtifactMetadataError result;
            result.m_assetId = id;
            result.m_source = source;
            result.m_code = ArtifactMetadataErrorCode::kIoError;
            result.m_message = message;
            return result;
        }
    } // namespace


    struct AssetManager::Impl final
    {
        FE_PROFILER_LOCK(Threading::Mutex, m_mutex);
        festd::unordered_dense_map<AssetID, AssetSlot*> m_slots;
        festd::unordered_dense_map<AssetID, Operation*> m_operations;
        festd::unordered_dense_map<Rtti::TypeID, Streamer*> m_streamers;
        festd::vector<AssetSlot*> m_ownedSlots;
        festd::vector<Operation*> m_ownedOperations;
        festd::vector<ReferenceGroup*> m_groups;
        festd::vector<Rc<WaitGroup>> m_jobs;
        festd::vector<Internal::AssetAcquisition*> m_waiters;
        festd::unordered_dense_set<Internal::AssetAcquisition*> m_acquisitions;
        ConcurrentOnceConsumedQueue m_payloadCompletions;
        DefaultStreamer m_defaultStreamer;
        bool m_shuttingDown = false;
        uint32_t m_retiredGenerationCount = 0;
        uint64_t m_nextOperationSerial = 0;
        festd::unordered_dense_map<AssetID, uint64_t> m_latestOperationSerial;
        festd::unordered_dense_map<AssetID, uint32_t> m_metadataReadCounts;
#if FE_DEVELOPMENT
        Rc<WaitGroup> m_entered;
        Rc<WaitGroup> m_resume;
#endif

        ~Impl();
        AssetSlot& Slot(AssetID id);
        Operation& GetOperation(AssetID id, festd::vector<Operation*>& starts);
        void Join(Internal::AssetAcquisition& request, AssetID id, Rtti::TypeID type, AssetID parentId,
                  festd::vector<Operation*>& starts);
        void Expand(Internal::AssetAcquisition& request, Operation& operation, festd::vector<Operation*>& starts);
        void CompleteMetadata(Operation& operation, ArtifactDecodeResult&& result);
        void StartMetadata(Operation& operation);
        void StartPayload(Operation& operation);
        void TrackJob(Rc<WaitGroup>&& job);
        void PruneCompletedJobs();
        void CancelIfUnused(Operation& operation);
        void Release(Internal::AssetAcquisition& request);
        void Finish(Internal::AssetAcquisition& request, AssetLoadResult result, festd::string_view error = {},
                    AssetID failedAssetId = AssetID::kNull);
        void DiscoveryDone(Internal::AssetAcquisition& request);
        void DetectGroups(Internal::AssetAcquisition& request);
        void DeleteIfUnused(Internal::AssetAcquisition* request);
        Streamer& GetStreamer(Operation& operation);
        void Tick();
#if FE_DEVELOPMENT
        AssetRequest Reload(AssetID id);
#endif
    };

    AssetManager::Impl* AssetManager::GImpl = nullptr;


    AssetManager::Impl::~Impl()
    {
        FE_PROFILER_ZONE();

        for (AssetSlot* slot : m_ownedSlots)
        {
            slot->m_completed.store(false, std::memory_order_release);
            slot->m_publicationGate.store(nullptr, std::memory_order_release);
        }

        for (Operation* operation : m_ownedOperations)
        {
            void* object = operation->m_candidate;
            if (operation->m_state == State::kPublished)
                object = operation->m_slot->m_instance.load();
            else if (operation->m_state == State::kRetiring)
                object = operation->m_retiredObject;

            if (operation->m_state == State::kFinalizing && object)
                GetStreamer(*operation).CancelFinalize(*operation->m_slot, object);

            DestroyObject(*operation, object);
            Memory::DefaultDelete(operation);
        }

        for (ReferenceGroup* group : m_groups)
        {
            if (group->m_gate)
                Memory::DefaultDelete(group->m_gate);

            Memory::DefaultDelete(group);
        }

        for (AssetSlot* slot : m_ownedSlots)
        {
            slot->m_instance.store(nullptr);
            const uint32_t previous = slot->m_lifetimeRefCount.fetch_sub(1, std::memory_order_acq_rel);
            FE_Assert(previous > 0, "Asset slot lifetime count underflow");
            if (previous == 1)
                Memory::DefaultDelete(slot);
        }
    }


    AssetSlot& AssetManager::Impl::Slot(AssetID id)
    {
        auto found = m_slots.find(id);
        if (found != m_slots.end())
            return *found->second;

        AssetSlot* slot = Memory::DefaultNew<AssetSlot>();
        slot->m_assetId = id;
        m_slots.insert({ id, slot });
        m_ownedSlots.push_back(slot);
        return *slot;
    }


    Operation& AssetManager::Impl::GetOperation(AssetID id, festd::vector<Operation*>& starts)
    {
        FE_PROFILER_ZONE();

        auto found = m_operations.find(id);
        if (found != m_operations.end())
        {
            Operation* operation = found->second;
            if (operation->m_state == State::kRetiring && operation->m_retiredObject)
            {
                operation->m_slot->m_instance.store(operation->m_retiredObject, std::memory_order_relaxed);
                operation->m_slot->m_completed.store(true, std::memory_order_release);
                operation->m_retiredObject = nullptr;
                operation->m_state = State::kPublished;
            }

            if (operation->m_state != State::kDormant && !operation->m_cancelRequested)
                return *operation;
        }

        Operation* operation = Memory::DefaultNew<Operation>();
        operation->m_id = id;
        operation->m_slot = &Slot(id);
        operation->m_serial = ++m_nextOperationSerial;
        m_latestOperationSerial.insert_or_assign(id, operation->m_serial);
        m_operations.insert_or_assign(id, operation);
        m_ownedOperations.push_back(operation);
        starts.push_back(operation);
        return *operation;
    }


    void AssetManager::Impl::Join(Internal::AssetAcquisition& request, AssetID id, Rtti::TypeID type, AssetID parentId,
                                  festd::vector<Operation*>& starts)
    {
        FE_PROFILER_ZONE();

        if (request.m_discovery.load() != AssetLoadResult::kPending)
            return;

        auto member = request.m_members.find(id);
        if (member != request.m_members.end())
        {
            if (type.IsValid() && member->second.IsValid() && member->second != type)
                Finish(request, AssetLoadResult::kFailed, "incompatible dependency types", id);
            else if (type.IsValid())
                member->second = type;

            return;
        }

        // Discovery is acquisition-local, while the underlying operation is shared. Recording membership before expanding the
        // operation prevents cycles and repeated dependencies from contributing residency more than once to this acquisition.
        Operation& operation = GetOperation(id, starts);
        request.m_members.insert({ id, type });
        request.m_parents.insert({ id, parentId });
        request.m_residentSlots.push_back(operation.m_slot);
        request.m_memberOperations.push_back(&operation);
        ++operation.m_interestCount;
        operation.m_slot->m_strongRefCount.fetch_add(1);

        if (operation.m_state == State::kFailed)
        {
            Finish(request, AssetLoadResult::kFailed, operation.m_error, id);
            return;
        }

        if (operation.m_state != State::kMetadata)
        {
            Expand(request, operation, starts);
            return;
        }

        ++request.m_pendingMetadata;
        ++request.m_workRefs;
        operation.m_subscribers.push_back(&request);
    }


    void AssetManager::Impl::Expand(Internal::AssetAcquisition& request, Operation& operation, festd::vector<Operation*>& starts)
    {
        FE_PROFILER_ZONE();

        Rtti::TypeID expected = request.m_members.find(operation.m_id)->second;
        if (expected.IsValid() && expected != operation.m_record.m_assetTypeId)
        {
            Finish(request, AssetLoadResult::kFailed, "dependency type does not match metadata", operation.m_id);
            return;
        }

        // Only hard dependencies extend the load closure. Soft and optional links are retained as identities and do not start
        // additional discovery.
        for (const ArtifactDependencyRecord& dependency : operation.m_record.m_dependencies)
        {
            if (dependency.m_kind == DependencyKind::kHard)
                Join(request, dependency.m_assetId, dependency.m_expectedTypeId, operation.m_id, starts);
        }
    }


    void AssetManager::Impl::CompleteMetadata(Operation& operation, ArtifactDecodeResult&& result)
    {
        FE_PROFILER_ZONE();

        festd::vector<Operation*> starts;
        bool payload = false;
        {
            std::lock_guard lock{ m_mutex };
            const bool wasCanceled = operation.m_cancelRequested && operation.m_interestCount == 0;
            if (wasCanceled)
            {
                operation.m_state = State::kDormant;
            }
            else if (result)
            {
                operation.m_record = std::move(result.value());
                operation.m_type = Rtti::TypeRegistry::FindType(operation.m_record.m_assetTypeId);
                auto current = m_operations.find(operation.m_id);
                const bool ownsSlotMetadata = current != m_operations.end() && current->second == &operation;
                if (ownsSlotMetadata)
                {
                    operation.m_slot->m_typeId = operation.m_record.m_assetTypeId;
                    operation.m_slot->m_currentArtifactId = operation.m_record.m_artifactId;
                }
                operation.m_state = State::kPayload;
                payload = true;

                // Reserve dependency operations before expanding subscribers so concurrent acquisitions and cycles observe one
                // operation. StartMetadata skips reservations that acquired no interest before dispatch.
                for (const ArtifactDependencyRecord& dependency : operation.m_record.m_dependencies)
                {
                    if (dependency.m_kind == DependencyKind::kHard)
                        GetOperation(dependency.m_assetId, starts);
                }
            }
            else
            {
                operation.m_error = Fmt::FixedFormat("asset {} metadata stage failed at '{}': {}",
                                                     operation.m_id,
                                                     result.error().m_source,
                                                     result.error().m_message);
                operation.m_state = State::kFailed;
            }

            // One decoded operation can unblock several acquisitions. Each subscriber expands its own deduplicated closure and
            // reaches discovery completion only after every newly found hard dependency has also completed metadata decoding.
            for (Internal::AssetAcquisition* request : operation.m_subscribers)
            {
                --request->m_pendingMetadata;
                if (request->m_discovery.load() == AssetLoadResult::kPending)
                {
                    if (operation.m_state == State::kFailed)
                        Finish(*request, AssetLoadResult::kFailed, operation.m_error, operation.m_id);
                    else if (operation.m_state == State::kDormant)
                        Finish(*request, AssetLoadResult::kCanceled, "asset operation was canceled", operation.m_id);
                    else
                        Expand(*request, operation, starts);

                    if (request->m_pendingMetadata == 0 && request->m_discovery.load() == AssetLoadResult::kPending)
                        DiscoveryDone(*request);
                }

                --request->m_workRefs;
                DeleteIfUnused(request);
            }
            operation.m_subscribers.clear();
        }

        // Starting I/O outside the manager lock avoids running dispatch or an immediately completing backend while shared graph
        // state is locked.
        for (Operation* start : starts)
            StartMetadata(*start);

        if (payload)
            StartPayload(operation);
    }


    void AssetManager::Impl::StartMetadata(Operation& operation)
    {
        FE_PROFILER_ZONE();

        {
            std::lock_guard lock{ m_mutex };
            if (operation.m_interestCount == 0)
            {
                operation.m_cancelRequested = true;
                operation.m_state = State::kDormant;
                return;
            }
            ++m_metadataReadCounts[operation.m_id];
        }

        // Pipeline stage 1: resolve and decode artifact metadata on a worker. CompleteMetadata performs recursive hard-dependency
        // discovery and starts the payload stage once this operation has a valid record.
        Rc<WaitGroup> job = WaitGroup::Create();
        Jobs::DispatchBackground(
            [this, &operation] {
                FE_PROFILER_ZONE_NAMED("Resolve artifact metadata");

#if FE_DEVELOPMENT
                Rc<WaitGroup> entered, resume;
                {
                    std::lock_guard lock{ m_mutex };
                    entered = m_entered;
                    resume = m_resume;
                }

                if (entered)
                    entered->Signal();
                if (resume)
                    resume->Wait();
#endif
                const ResolvedDataSource source = ArtifactStore::ResolveMeta(operation.m_id);
                festd::pmr::vector<std::byte> bytes;
                Rc<WaitGroup> done = WaitGroup::Create();
                Async::Batch batch(source, done.Get());
                batch.ReadAppendToEnd(bytes);

                Rc<Async::IController> controller = Async::Read(batch);
                {
                    std::lock_guard lock{ m_mutex };
                    operation.m_activeController = controller;
                    if (operation.m_cancelRequested)
                        controller->Cancel();
                }
                done->Wait();

                {
                    std::lock_guard lock{ m_mutex };
                    operation.m_activeController.Reset();
                }

                if (controller->GetStatus() != Async::Status::kSucceeded)
                {
                    CompleteMetadata(operation,
                                     festd::unexpected(IoError(operation.m_id, source.m_filePath, "metadata read failed")));
                    return;
                }

                ArtifactResolutionContext context{ operation.m_id, ArtifactID::kNull, source };
                CompleteMetadata(operation, ArtifactStore::Decode(bytes, context));
            },
            job.Get());

        TrackJob(std::move(job));
    }


    void AssetManager::Impl::StartPayload(Operation& operation)
    {
        FE_PROFILER_ZONE();

        {
            std::lock_guard lock{ m_mutex };
            if (operation.m_cancelRequested && operation.m_interestCount == 0)
            {
                operation.m_state = State::kDormant;
                return;
            }
        }

        // Pipeline stage 2: read all planned chunks, decompress them into one logical payload, validate every decoded chunk, and
        // deserialize a private candidate. Publication is deliberately deferred to Tick.
        Rc<WaitGroup> job = WaitGroup::Create();
        Jobs::DispatchBackground(
            [this, &operation] {
                FE_PROFILER_ZONE_NAMED("Read chunks");

                const ArtifactPayloadRecord& payload = operation.m_record.m_payloads.front();
                festd::pmr::vector<std::byte> bytes;

                size_t size = 0;
                for (const ArtifactChunkRecord& chunk : payload.m_chunks)
                    size += static_cast<size_t>(chunk.m_uncompressedSize);

                FE_Assert(size <= Constants::kMaxU32);
                bytes.reserve(static_cast<uint32_t>(size));

                Rc<WaitGroup> done = WaitGroup::Create();
                Async::Batch batch(payload.m_resolvedDataSource, done.Get());
                for (const ArtifactChunkRecord& chunk : payload.m_chunks)
                {
                    batch.ReadAppend(bytes,
                                     chunk.m_compressionMethod,
                                     chunk.m_compressedSize,
                                     chunk.m_uncompressedSize,
                                     chunk.m_offsetInPayload);
                }

                Rc<Async::IController> controller = Async::Read(batch);
                {
                    std::lock_guard lock{ m_mutex };
                    operation.m_activeController = controller;
                    if (operation.m_cancelRequested)
                        controller->Cancel();
                }
                done->Wait();
                {
                    std::lock_guard lock{ m_mutex };
                    operation.m_activeController.Reset();
                }

                if (controller->GetStatus() != Async::Status::kSucceeded)
                    operation.m_error = Fmt::FixedFormat("asset {} artifact {} payload read failed at '{}'",
                                                         operation.m_id,
                                                         operation.m_record.m_artifactId,
                                                         payload.m_resolvedDataSource.m_filePath);

                size_t offset = 0;
                for (const ArtifactChunkRecord& chunk : payload.m_chunks)
                {
                    if (operation.m_error.empty()
                        && Crc32::Compute(bytes.data() + offset, chunk.m_uncompressedSize) != chunk.m_checksum.m_current)
                    {
                        operation.m_error = "primary payload checksum mismatch";
                    }

                    offset += chunk.m_uncompressedSize;
                }

                const bool hasDeserializer = operation.m_type && operation.m_type->m_deserialize;
                const bool isTrivial =
                    operation.m_type && (operation.m_type->m_flags & Rtti::TypeFlags::kTrivial) != Rtti::TypeFlags::kNone;
                const bool canConstruct = operation.m_type && (operation.m_type->m_defaultConstructor || isTrivial);
                if (operation.m_error.empty() && (!hasDeserializer || !canConstruct))
                    operation.m_error = "asset type is not constructible and deserializable";

                if (operation.m_error.empty())
                {
                    operation.m_candidate = Memory::DefaultAllocate(operation.m_type->m_size, operation.m_type->m_alignment);
                    if (operation.m_type->m_defaultConstructor)
                        operation.m_type->m_defaultConstructor(operation.m_candidate);
                    else
                        std::memset(operation.m_candidate, 0, operation.m_type->m_size);

                    ReadOnlyMemoryStream stream(bytes);
                    Serialization::ResultCode result;

                    uint32_t firstContentByte = 0;
                    while (firstContentByte < bytes.size() && std::isspace(static_cast<unsigned char>(bytes[firstContentByte])))
                    {
                        ++firstContentByte;
                    }

                    const bool isJson = firstContentByte < bytes.size() && static_cast<char>(bytes[firstContentByte]) == '{';
                    if (isJson)
                    {
                        Serialization::JsonFormat format;
                        Serialization::DeserializationContext context(&stream, format);
                        result = context.Load(*operation.m_type, operation.m_candidate);
                    }
                    else
                    {
                        Serialization::TaggedBinaryFormat format;
                        Serialization::DeserializationContext context(&stream, format);
                        result = context.Load(*operation.m_type, operation.m_candidate);
                    }

                    if (result != Serialization::ResultCode::kSuccess)
                    {
                        operation.m_error =
                            Fmt::FixedFormat("primary payload deserialization failed ({})", festd::to_underlying(result));
                        DestroyObject(operation, operation.m_candidate);
                        operation.m_candidate = nullptr;
                    }
                }
                m_payloadCompletions.Enqueue(&operation);
            },
            job.Get());

        TrackJob(std::move(job));
    }


    void AssetManager::Impl::TrackJob(Rc<WaitGroup>&& job)
    {
        FE_PROFILER_ZONE();

        std::lock_guard lock{ m_mutex };
        PruneCompletedJobs();
        m_jobs.push_back(std::move(job));
    }


    void AssetManager::Impl::PruneCompletedJobs()
    {
        for (uint32_t index = 0; index < m_jobs.size();)
        {
            if (m_jobs[index]->IsSignaled())
            {
                m_jobs[index] = std::move(m_jobs.back());
                m_jobs.pop_back();
            }
            else
                ++index;
        }
    }


    void AssetManager::Impl::Release(Internal::AssetAcquisition& request)
    {
        FE_PROFILER_ZONE();

        if (request.m_released)
            return;

        request.m_released = true;
        for (uint32_t index = 0; index < request.m_residentSlots.size(); ++index)
        {
            AssetSlot* slot = request.m_residentSlots[index];
            Operation* operation = request.m_memberOperations[index];
            uint32_t previous = slot->m_strongRefCount.fetch_sub(1);
            FE_Assert(previous > 0, "Asset residency count underflow");
            FE_Assert(operation->m_interestCount > 0, "Asset operation interest count underflow");
            --operation->m_interestCount;
            CancelIfUnused(*operation);
        }
    }


    void AssetManager::Impl::CancelIfUnused(Operation& operation)
    {
        if (operation.m_interestCount != 0 || operation.m_state == State::kPublished || operation.m_state == State::kRetiring
            || operation.m_state == State::kDormant || operation.m_state == State::kFailed)
        {
            return;
        }

        operation.m_cancelRequested = true;
        if (operation.m_activeController)
            operation.m_activeController->Cancel();
    }


    void AssetManager::Impl::Finish(Internal::AssetAcquisition& request, AssetLoadResult result, festd::string_view error,
                                    AssetID failedAssetId)
    {
        FE_PROFILER_ZONE();

        if (request.m_result.load() != AssetLoadResult::kPending)
            return;

        if (!error.empty())
        {
            if (request.m_root && failedAssetId.IsValid())
            {
                festd::inline_vector<AssetID, 8> path;
                AssetID current = failedAssetId;
                while (current.IsValid() && path.size() <= request.m_parents.size())
                {
                    path.push_back(current);
                    if (current == request.m_root->m_assetId)
                        break;

                    auto parent = request.m_parents.find(current);
                    current = parent == request.m_parents.end() ? AssetID::kNull : parent->second;
                }

                request.m_error = "root acquisition ";
                const auto appendId = [&request](const AssetID id) {
                    const auto text = Fmt::FixedFormat("{}", id);
                    request.m_error.append(text.data(), text.size());
                };
                if (!path.empty() && path.back() == request.m_root->m_assetId)
                {
                    for (uint32_t index = path.size(); index > 0; --index)
                    {
                        if (index != path.size())
                            request.m_error.append(" -> ");
                        appendId(path[index - 1]);
                    }
                }
                else
                {
                    appendId(request.m_root->m_assetId);
                    request.m_error.append(" -> ");
                    appendId(failedAssetId);
                }
                request.m_error.append(": ");
                request.m_error.append(error.data(), error.size());
            }
            else if (request.m_root)
            {
                request.m_error = Fmt::FixedFormat("root acquisition {}: ", request.m_root->m_assetId);
                request.m_error.append(error.data(), error.size());
            }
            else
                request.m_error.assign(error.data(), error.size());
        }

        if (result != AssetLoadResult::kSucceeded)
            Release(request);

        request.m_result.store(result, std::memory_order_release);
        if (request.m_discovery.load() == AssetLoadResult::kPending)
        {
            request.m_discovery.store(result, std::memory_order_release);
            request.m_discoveryEvent->Signal();
        }

        request.m_completionEvent->Signal();
    }


    void AssetManager::Impl::DiscoveryDone(Internal::AssetAcquisition& request)
    {
        FE_PROFILER_ZONE();

        // Discovery completion means that the complete hard metadata closure is known. It is intentionally separate from request
        // completion: payload reads, finalization, and publication may all still be pending.
        DetectGroups(request);
        request.m_discovery.store(AssetLoadResult::kSucceeded, std::memory_order_release);
        request.m_discoveryEvent->Signal();
        ++request.m_workRefs;
        m_waiters.push_back(&request);
    }


    void AssetManager::Impl::DetectGroups(Internal::AssetAcquisition& request)
    {
        FE_PROFILER_ZONE();

        // Find strongly connected components in the hard-dependency graph. A cyclic component must be treated as one publication
        // group because its candidates contain links to one another and no member may become observable before all members exist.
        struct Node
        {
            Operation* m_operation;
            uint32_t m_index = kInvalidIndex;
            uint32_t m_low = kInvalidIndex;
            bool m_active = false;
        };

        festd::vector<Node> nodes;
        festd::unordered_dense_map<AssetID, uint32_t> indices;
        for (const auto& member : request.m_members)
        {
            indices.insert({ member.first, nodes.size() });
            Operation* operation = m_operations.find(member.first)->second;
            if (member.first == request.m_root->m_assetId && request.m_targetOperation)
                operation = static_cast<Operation*>(request.m_targetOperation);
            nodes.push_back({ operation });
        }

        festd::vector<uint32_t> stack;
        uint32_t next = 0;
        const auto visit = [&](auto&& self, uint32_t index) -> void {
            FE_PROFILER_ZONE_NAMED("Visit");

            Node& node = nodes[index];
            node.m_index = node.m_low = next++;
            node.m_active = true;
            stack.push_back(index);
            for (const ArtifactDependencyRecord& dependency : node.m_operation->m_record.m_dependencies)
            {
                if (dependency.m_kind != DependencyKind::kHard)
                    continue;

                auto found = indices.find(dependency.m_assetId);
                if (found == indices.end())
                    continue;

                Node& target = nodes[found->second];
                if (target.m_index == kInvalidIndex)
                {
                    self(self, found->second);
                    node.m_low = Math::Min(node.m_low, target.m_low);
                }
                else if (target.m_active)
                {
                    node.m_low = Math::Min(node.m_low, target.m_index);
                }
            }

            if (node.m_low != node.m_index)
                return;

            festd::inline_vector<uint32_t, 8> component;
            for (;;)
            {
                uint32_t member = stack.back();
                stack.pop_back();
                nodes[member].m_active = false;
                component.push_back(member);
                if (member == index)
                    break;
            }

            bool cyclic = component.size() > 1;
            if (!cyclic)
            {
                for (const ArtifactDependencyRecord& dependency : node.m_operation->m_record.m_dependencies)
                {
                    cyclic |= dependency.m_kind == DependencyKind::kHard && dependency.m_assetId == node.m_operation->m_id;
                }
            }

            if (!cyclic)
                return;

            ReferenceGroup* group = nullptr;
            bool hasPublishedMember = false;
            for (uint32_t member : component)
            {
                Operation* operation = nodes[member].m_operation;
                hasPublishedMember |= operation->m_state == State::kPublished;
                if (operation->m_publicationGroup)
                {
                    FE_Assert(group == nullptr || group == operation->m_publicationGroup,
                              "Asset operations belong to incompatible publication groups");
                    group = operation->m_publicationGroup;
                }
            }

            if (hasPublishedMember)
                return;

            if (!group)
            {
                group = Memory::DefaultNew<ReferenceGroup>();
                m_groups.push_back(group);
            }

            for (uint32_t member : component)
            {
                Operation* operation = nodes[member].m_operation;
                if (!operation->m_publicationGroup)
                {
                    operation->m_publicationGroup = group;
                    group->m_members.push_back(operation);
                }
            }
        };

        for (uint32_t index = 0; index < nodes.size(); ++index)
        {
            if (nodes[index].m_index == kInvalidIndex)
                visit(visit, index);
        }
    }


    Streamer& AssetManager::Impl::GetStreamer(Operation& operation)
    {
        auto found = m_streamers.find(operation.m_record.m_assetTypeId);
        return found == m_streamers.end() ? m_defaultStreamer : *found->second;
    }


    void AssetManager::Impl::Tick()
    {
        // Pipeline stage 3: transfer background deserialization results to the main-thread state machine. Workers only enqueue
        // completions, so streamer callbacks and publication always happen from Tick.
        std::unique_lock lock{ m_mutex };
        ConcurrentOnceConsumedQueue::Node* completion = m_payloadCompletions.DequeueAll();
        while (completion)
        {
            Operation* operation = static_cast<Operation*>(completion);
            completion = completion->m_next;
            const bool wasAbandoned = operation->m_cancelRequested && operation->m_interestCount == 0;
            operation->m_state =
                wasAbandoned ? State::kDormant : (operation->m_error.empty() ? State::kCandidate : State::kFailed);
            if (operation->m_state != State::kCandidate && operation->m_candidate)
            {
                void* candidate = operation->m_candidate;
                operation->m_candidate = nullptr;
                lock.unlock();
                DestroyObject(*operation, candidate);
                lock.lock();
            }
        }

        // Operations are manager-owned until shutdown. Process only the prefix visible at tick start; background discovery may
        // append more operations while callbacks run without invalidating the stable operation objects.
        const uint32_t operationCount = m_ownedOperations.size();
        festd::unordered_dense_set<ReferenceGroup*> processedGroups;

        const auto failGroup = [this, &lock](const festd::span<Operation* const> members, const festd::string_view error) {
            festd::inline_vector<Operation*, 8> destroyOperations;
            festd::inline_vector<void*, 8> destroyObjects;
            festd::inline_vector<Operation*, 8> cancelOperations;
            festd::inline_vector<Streamer*, 8> cancelStreamers;
            festd::inline_vector<void*, 8> cancelObjects;
            for (Operation* member : members)
            {
                if (member->m_state == State::kPublished)
                    continue;

                if (member->m_state == State::kFinalizing && member->m_candidate)
                {
                    cancelOperations.push_back(member);
                    cancelStreamers.push_back(&GetStreamer(*member));
                    cancelObjects.push_back(member->m_candidate);
                }

                if (member->m_error.empty())
                    member->m_error = error;
                member->m_state = State::kFailed;
                if (member->m_candidate)
                {
                    destroyOperations.push_back(member);
                    destroyObjects.push_back(member->m_candidate);
                    member->m_candidate = nullptr;
                }
            }

            lock.unlock();
            for (uint32_t index = 0; index < cancelOperations.size(); ++index)
            {
                Operation* operation = cancelOperations[index];
                cancelStreamers[index]->CancelFinalize(*operation->m_slot, cancelObjects[index]);
            }
            for (uint32_t index = 0; index < destroyOperations.size(); ++index)
                DestroyObject(*destroyOperations[index], destroyObjects[index]);
            lock.lock();
        };

        for (uint32_t operationIndex = 0; operationIndex < operationCount; ++operationIndex)
        {
            Operation* seed = m_ownedOperations[operationIndex];
            if (seed->m_state != State::kCandidate && seed->m_state != State::kFinalizing)
                continue;

            ReferenceGroup* group = seed->m_publicationGroup;
            if (group && !processedGroups.insert(group).second)
                continue;

            festd::inline_vector<Operation*, 8> members;
            if (!group)
            {
                members.push_back(seed);
            }
            else
            {
                members.insert(members.end(), group->m_members.begin(), group->m_members.end());
            }

            bool wasSuperseded = false;
            for (Operation* member : members)
            {
                auto latest = m_latestOperationSerial.find(member->m_id);
                wasSuperseded |= latest == m_latestOperationSerial.end() || latest->second != member->m_serial;
                wasSuperseded |= member->m_cancelRequested;
            }
            if (wasSuperseded)
            {
                failGroup(members, "asset operation was superseded or canceled");
                continue;
            }

            // A reference group advances as a unit. In particular, a hard cycle cannot finalize or publish while one of its
            // candidates is still being produced by a worker.
            bool candidatesReady = true;
            bool groupFailed = false;
            bool orderedCycle = false;
            for (Operation* member : members)
            {
                candidatesReady &= member->m_state == State::kCandidate || member->m_state == State::kFinalizing
                    || member->m_state == State::kPublished;
                groupFailed |= member->m_state == State::kFailed;
                orderedCycle |= group && GetStreamer(*member).RequiresFinalizedDependencies();
            }

            if (groupFailed)
            {
                failGroup(members, "asset reference group member failed");
                continue;
            }

            if (!candidatesReady)
                continue;

            // A streamer that needs already-finalized dependencies imposes an ordering edge. Such an edge cannot be satisfied
            // inside a cycle, so fail deterministically instead of leaving the group pending forever.
            if (orderedCycle)
            {
                failGroup(members, "finalization-order cycle is unsupported");
                continue;
            }

            // Dependencies inside this group are candidates by construction. External dependencies gate publication. Finalizers
            // that do not request dependency ordering may overlap that wait.
            bool dependenciesReady = true;
            bool dependencyFailed = false;
            festd::inline_vector<uint8_t, 8> memberDependenciesValidated;
            festd::inline_vector<uint8_t, 8> memberDependenciesReady;
            for (Operation* member : members)
            {
                bool currentDependenciesValidated = true;
                bool currentDependenciesReady = true;
                for (const ArtifactDependencyRecord& dependency : member->m_record.m_dependencies)
                {
                    if (dependency.m_kind != DependencyKind::kHard)
                        continue;

                    auto targetIt = m_operations.find(dependency.m_assetId);
                    if (targetIt == m_operations.end())
                    {
                        dependencyFailed = true;
                        currentDependenciesValidated = false;
                        currentDependenciesReady = false;
                        continue;
                    }

                    Operation* target = targetIt->second;
                    const bool sameGroup = group && target->m_publicationGroup == group;
                    const bool typeIsKnown = target->m_slot->m_typeId.IsValid();
                    const bool typeMismatch = typeIsKnown && target->m_slot->m_typeId != dependency.m_expectedTypeId;
                    if (target->m_state == State::kFailed || typeMismatch)
                    {
                        dependencyFailed = true;
                        currentDependenciesValidated = false;
                        currentDependenciesReady = false;
                    }
                    else if (!typeIsKnown)
                    {
                        currentDependenciesValidated = false;
                        currentDependenciesReady = false;
                    }
                    else if (!sameGroup && target->m_state != State::kPublished)
                    {
                        currentDependenciesReady = false;
                    }
                }

                memberDependenciesValidated.push_back(currentDependenciesValidated);
                memberDependenciesReady.push_back(currentDependenciesReady);
                dependenciesReady &= currentDependenciesReady;
            }

            if (dependencyFailed)
            {
                failGroup(members, "hard dependency failed or has an incompatible type");
                continue;
            }

            // Pipeline stage 4: start or poll type-specific finalization. Streamer callbacks execute without the registry lock,
            // while operation state changes remain synchronized with concurrent requests and discovery.
            bool finalized = true;
            bool failed = false;
            for (uint32_t memberIndex = 0; memberIndex < members.size(); ++memberIndex)
            {
                Operation* member = members[memberIndex];
                if (member->m_state == State::kPublished)
                    continue;

                Streamer* streamer = &GetStreamer(*member);
                const bool isStarting = member->m_state == State::kCandidate;
                const bool dependenciesValidated = memberDependenciesValidated[memberIndex] != 0;
                const bool dependencyOrderSatisfied = memberDependenciesReady[memberIndex] != 0;
                bool mayStart = dependenciesValidated;
                if (streamer->RequiresFinalizedDependencies())
                    mayStart &= dependencyOrderSatisfied;
                const bool shouldInvoke = (isStarting && mayStart)
                    || (member->m_state == State::kFinalizing && member->m_finalize == AssetFinalizeResult::kPending);
                if (shouldInvoke)
                {
                    if (isStarting)
                        member->m_state = State::kFinalizing;

                    AssetSlot* slot = member->m_slot;
                    void* candidate = member->m_candidate;
                    lock.unlock();
                    const AssetFinalizeResult result =
                        isStarting ? streamer->FinalizeAssetLoading(*slot, candidate) : streamer->PollFinalize(*slot, candidate);
                    lock.lock();
                    member->m_finalize = result;
                }

                finalized &= member->m_finalize == AssetFinalizeResult::kSucceeded;
                failed |= member->m_finalize == AssetFinalizeResult::kFailed;
            }

            if (failed)
            {
                failGroup(members, "asset finalization failed");
                continue;
            }

            if (!finalized || !dependenciesReady)
                continue;

            // Pipeline stage 5: commit the group. Completed is published before readers consult the gate; the shared release-store
            // then opens one visibility boundary only after every candidate pointer is installed.
            AssetPublicationGate* gate = nullptr;
            if (group)
            {
                if (!group->m_gate)
                    group->m_gate = Memory::DefaultNew<AssetPublicationGate>();
                gate = group->m_gate;
            }
            for (Operation* member : members)
            {
                if (member->m_state != State::kPublished)
                {
                    auto currentIt = m_operations.find(member->m_id);
                    Operation* current = currentIt == m_operations.end() ? nullptr : currentIt->second;
                    if (current && current != member && current->m_state == State::kPublished)
                    {
                        current->m_retiredObject = member->m_slot->m_instance.load(std::memory_order_acquire);
                        current->m_retirementGroup = nullptr;
                        current->m_state = State::kRetiring;
                    }

                    m_operations.insert_or_assign(member->m_id, member);
                    member->m_slot->m_typeId = member->m_record.m_assetTypeId;
                    member->m_slot->m_currentArtifactId = member->m_record.m_artifactId;

                    for (const ArtifactDependencyRecord& dependency : member->m_record.m_dependencies)
                    {
                        if (dependency.m_kind != DependencyKind::kHard)
                            continue;
                        Operation* target = m_operations.find(dependency.m_assetId)->second;
                        const bool sameGroup = group && target->m_publicationGroup == group;
                        if (!sameGroup)
                        {
                            target->m_readHolds.fetch_add(1, std::memory_order_relaxed);
                            member->m_dependencyHolds.push_back(target);
                        }
                    }

                    member->m_slot->m_publicationGate.store(gate, std::memory_order_relaxed);
                    member->m_slot->m_instance.store(member->m_candidate, std::memory_order_relaxed);
                    member->m_candidate = nullptr;
                    member->m_slot->m_generation.fetch_add(1, std::memory_order_relaxed);
                    member->m_slot->m_completed.store(true, std::memory_order_release);
                    member->m_state = State::kPublished;
                }
            }

            if (gate)
                gate->m_isOpen.store(true, std::memory_order_release);
        }

        // Complete acquisitions only after every member of their hard closure is published, or as soon as any member fails.
        for (uint32_t index = 0; index < m_waiters.size();)
        {
            Internal::AssetAcquisition* request = m_waiters[index];
            if (request->m_result.load(std::memory_order_acquire) != AssetLoadResult::kPending)
            {
                --request->m_workRefs;
                m_waiters.erase(m_waiters.begin() + index);
                DeleteIfUnused(request);
                continue;
            }

            bool failed = false, published = true;
            festd::string_view error;
            AssetID failedAssetId = AssetID::kNull;
            for (const auto& member : request->m_members)
            {
                Operation* operation = m_operations.find(member.first)->second;
                if (member.first == request->m_root->m_assetId && request->m_targetOperation)
                    operation = static_cast<Operation*>(request->m_targetOperation);
                failed |= operation->m_state == State::kFailed;
                published &= operation->m_state == State::kPublished;
                if (operation->m_state == State::kFailed)
                {
                    error = operation->m_error;
                    failedAssetId = member.first;
                }
            }

            if (!failed && !published)
            {
                ++index;
                continue;
            }

            Finish(*request, failed ? AssetLoadResult::kFailed : AssetLoadResult::kSucceeded, error, failedAssetId);
            --request->m_workRefs;
            m_waiters.erase(m_waiters.begin() + index);
            DeleteIfUnused(request);
        }

        // Zero residency demand makes a generation eligible for eviction. Close admission and detach an entire cyclic group as
        // one action; generation readers that entered before this point keep the detached objects alive.
        processedGroups.clear();
        for (uint32_t operationIndex = 0; operationIndex < operationCount; ++operationIndex)
        {
            Operation* seed = m_ownedOperations[operationIndex];
            if (seed->m_state != State::kPublished || seed->m_slot->m_strongRefCount.load() != 0)
                continue;

            auto current = m_operations.find(seed->m_id);
            if (current == m_operations.end() || current->second != seed)
                continue;

            ReferenceGroup* group = seed->m_publicationGroup;
            bool intactGroup = group != nullptr;
            if (group)
            {
                for (Operation* member : group->m_members)
                {
                    auto memberCurrent = m_operations.find(member->m_id);
                    intactGroup &= memberCurrent != m_operations.end() && memberCurrent->second == member
                        && member->m_state == State::kPublished;
                }
            }

            if (intactGroup && !processedGroups.insert(group).second)
                continue;

            festd::inline_vector<Operation*, 8> members;
            if (intactGroup)
            {
                for (Operation* member : group->m_members)
                    members.push_back(member);
            }
            else
                members.push_back(seed);

            bool mayRetire = true;
            for (Operation* member : members)
                mayRetire &= member->m_state == State::kPublished && member->m_slot->m_strongRefCount.load() == 0;
            if (!mayRetire)
                continue;

            for (Operation* member : members)
            {
                member->m_slot->m_completed.store(false, std::memory_order_release);
                member->m_retiredObject = member->m_slot->m_instance.exchange(nullptr, std::memory_order_acq_rel);
                member->m_retirementGroup = intactGroup ? group : nullptr;
                member->m_state = State::kRetiring;
            }
        }

        // Concrete destruction is a main-thread operation. Dependents and cyclic groups are retained until every admitted reader
        // in the retirement unit has left, so destructors cannot observe already-reclaimed required objects.
        processedGroups.clear();
        for (uint32_t operationIndex = 0; operationIndex < operationCount; ++operationIndex)
        {
            Operation* seed = m_ownedOperations[operationIndex];
            if (seed->m_state != State::kRetiring)
                continue;

            ReferenceGroup* group = seed->m_retirementGroup;
            if (group && !processedGroups.insert(group).second)
                continue;

            festd::inline_vector<Operation*, 8> members;
            bool readersDrained = true;
            if (group)
            {
                for (Operation* member : group->m_members)
                    members.push_back(member);
            }
            else
                members.push_back(seed);

            for (Operation* member : members)
                readersDrained &= member->m_state == State::kRetiring && member->m_readHolds.load(std::memory_order_acquire) == 0;

            if (!readersDrained)
                continue;

            for (Operation* member : members)
            {
                void* object = member->m_retiredObject;
                member->m_retiredObject = nullptr;
                member->m_retirementGroup = nullptr;
                member->m_state = State::kDormant;
                ++m_retiredGenerationCount;
                lock.unlock();
                DestroyObject(*member, object);
                for (Operation* dependency : member->m_dependencyHolds)
                {
                    const uint32_t previous = dependency->m_readHolds.fetch_sub(1, std::memory_order_release);
                    FE_Assert(previous > 0, "Asset generation dependency hold underflow");
                }
                member->m_dependencyHolds.clear();
                lock.lock();
            }
        }
    }


    void AssetManager::Impl::DeleteIfUnused(Internal::AssetAcquisition* request)
    {
        if (request->m_externalRefs.load() == 0 && request->m_workRefs == 0)
        {
            m_acquisitions.erase(request);
            Memory::DefaultDelete(request);
        }
    }


#if FE_DEVELOPMENT
    AssetRequest AssetManager::Impl::Reload(AssetID id)
    {
        Internal::AssetAcquisition* request = nullptr;
        Operation* operation = nullptr;
        {
            std::lock_guard lock{ m_mutex };
            FE_Assert(!m_shuttingDown);
            auto current = m_operations.find(id);
            if (current == m_operations.end() || current->second->m_state != State::kPublished
                || current->second->m_publicationGroup)
            {
                return {};
            }

            request = Memory::DefaultNew<Internal::AssetAcquisition>();
            operation = Memory::DefaultNew<Operation>();
            AssetSlot& slot = Slot(id);
            operation->m_id = id;
            operation->m_slot = &slot;
            operation->m_serial = ++m_nextOperationSerial;
            m_latestOperationSerial.insert_or_assign(id, operation->m_serial);
            m_ownedOperations.push_back(operation);

            request->m_root = &slot;
            request->m_targetOperation = operation;
            request->m_members.insert({ id, slot.m_typeId });
            request->m_parents.insert({ id, AssetID::kNull });
            request->m_residentSlots.push_back(&slot);
            request->m_memberOperations.push_back(operation);
            ++operation->m_interestCount;
            slot.m_strongRefCount.fetch_add(1);
            ++request->m_pendingMetadata;
            ++request->m_workRefs;
            operation->m_subscribers.push_back(request);
            m_acquisitions.insert(request);
        }
        StartMetadata(*operation);
        return AssetRequest(request);
    }
#endif


    namespace Internal
    {
        const void* AcquireAssetGeneration(AssetSlot* slot, void*& generationToken)
        {
            generationToken = nullptr;
            if (!slot || !AssetManager::GImpl)
                return nullptr;

            std::lock_guard lock{ AssetManager::GImpl->m_mutex };
            auto found = AssetManager::GImpl->m_operations.find(slot->m_assetId);
            if (found == AssetManager::GImpl->m_operations.end())
                return nullptr;

            Operation* operation = found->second;
            if (operation->m_slot != slot || operation->m_state != State::kPublished)
                return nullptr;

            void* instance = slot->m_instance.load(std::memory_order_acquire);
            if (!instance)
                return nullptr;

            operation->m_readHolds.fetch_add(1, std::memory_order_relaxed);
            generationToken = operation;
            return instance;
        }


        void ReleaseAssetGeneration(void* generationToken)
        {
            if (!generationToken)
                return;

            Operation* operation = static_cast<Operation*>(generationToken);
            const uint32_t previous = operation->m_readHolds.fetch_sub(1, std::memory_order_release);
            FE_Assert(previous > 0, "Asset generation read count underflow");
        }


        AssetSlot* FindAssetSlot(AssetID id)
        {
            return AssetManager::FindAssetSlot(id);
        }
    } // namespace Internal


    AssetFinalizeResult DefaultStreamer::FinalizeAssetLoading(AssetSlot&, void*)
    {
        return AssetFinalizeResult::kSucceeded;
    }


    AssetFinalizeResult DefaultStreamer::PollFinalize(AssetSlot&, void*)
    {
        return AssetFinalizeResult::kSucceeded;
    }


    void DefaultStreamer::CancelFinalize(AssetSlot&, void*) {}


    void AssetManager::Init()
    {
        FE_PROFILER_ZONE();

        FE_Assert(!GImpl);
        GImpl = Memory::DefaultNew<Impl>();
    }


    void AssetManager::Shutdown()
    {
        FE_PROFILER_ZONE();

        FE_Assert(GImpl && Threading::IsMainThread());
        {
            std::lock_guard lock{ GImpl->m_mutex };
            GImpl->m_shuttingDown = true;
        }

        for (;;)
        {
            festd::vector<Rc<WaitGroup>> jobs;
            {
                std::lock_guard lock{ GImpl->m_mutex };
                GImpl->PruneCompletedJobs();
                if (GImpl->m_jobs.empty())
                    break;

                jobs = GImpl->m_jobs;
            }
            WaitGroup::WaitAll(jobs);
        }
        GImpl->Tick();
        {
            std::lock_guard lock{ GImpl->m_mutex };
            festd::vector<Internal::AssetAcquisition*> abandoned;
            for (Internal::AssetAcquisition* request : GImpl->m_acquisitions)
            {
                if (request->m_result.load() == AssetLoadResult::kPending)
                    GImpl->Finish(*request, AssetLoadResult::kCanceled, "asset manager shut down");
                GImpl->Release(*request);
                request->m_root = nullptr;
                request->m_managerAlive.store(false, std::memory_order_release);
                request->m_workRefs = 0;
                if (request->m_externalRefs.load() == 0)
                    abandoned.push_back(request);
            }
            GImpl->m_waiters.clear();
            for (Internal::AssetAcquisition* request : abandoned)
            {
                GImpl->m_acquisitions.erase(request);
                Memory::DefaultDelete(request);
            }
        }
        Memory::DefaultDelete(GImpl);
        GImpl = nullptr;
    }


    AssetRequest AssetManager::LoadAsset(AssetID id)
    {
        return LoadAsset(id, Rtti::TypeID::kNull);
    }


    AssetRequest AssetManager::LoadAsset(AssetID id, Rtti::TypeID expectedTypeId)
    {
        FE_PROFILER_ZONE();

        FE_Assert(GImpl);
        if (!id.IsValid())
            return {};

        auto* request = Memory::DefaultNew<Internal::AssetAcquisition>();
        festd::vector<Operation*> starts;
        {
            std::lock_guard lock{ GImpl->m_mutex };
            FE_Assert(!GImpl->m_shuttingDown);
            request->m_root = &GImpl->Slot(id);
            GImpl->Join(*request, id, expectedTypeId, AssetID::kNull, starts);
            GImpl->m_acquisitions.insert(request);
            if (request->m_pendingMetadata == 0 && request->m_discovery.load() == AssetLoadResult::kPending)
                GImpl->DiscoveryDone(*request);
        }

        for (Operation* operation : starts)
            GImpl->StartMetadata(*operation);

        return AssetRequest(request);
    }


#if FE_DEVELOPMENT
    AssetRequest AssetManager::ReloadAsset(AssetID id)
    {
        FE_PROFILER_ZONE();

        FE_Assert(GImpl && id.IsValid());
        return GImpl->Reload(id);
    }
#endif


    AssetSlot* AssetManager::FindAssetSlot(AssetID id)
    {
        if (!GImpl)
            return nullptr;

        std::lock_guard lock{ GImpl->m_mutex };
        auto found = GImpl->m_slots.find(id);
        return found == GImpl->m_slots.end() ? nullptr : found->second;
    }


    void AssetManager::RegisterStreamer(Rtti::TypeID type, Streamer* streamer)
    {
        FE_Assert(GImpl && streamer);
        std::lock_guard lock{ GImpl->m_mutex };
        GImpl->m_streamers.insert_or_assign(type, streamer);
    }


    void AssetManager::Tick()
    {
        FE_PROFILER_ZONE();

        FE_Assert(GImpl && Threading::IsMainThread());
        GImpl->Tick();
    }


#if FE_DEVELOPMENT
    void AssetManager::SetDiscoveryBarrierForTests(WaitGroup* entered, WaitGroup* resume)
    {
        std::lock_guard lock{ GImpl->m_mutex };
        GImpl->m_entered = entered;
        GImpl->m_resume = resume;
    }


    uint32_t AssetManager::GetMetadataReadCountForTests(AssetID id)
    {
        std::lock_guard lock{ GImpl->m_mutex };
        auto found = GImpl->m_metadataReadCounts.find(id);
        return found == GImpl->m_metadataReadCounts.end() ? 0 : found->second;
    }


    bool AssetManager::IsRetiringForTests(AssetID id)
    {
        std::lock_guard lock{ GImpl->m_mutex };
        auto found = GImpl->m_operations.find(id);
        return found != GImpl->m_operations.end() && found->second->m_state == State::kRetiring;
    }


    uint32_t AssetManager::GetRetiredGenerationCountForTests()
    {
        std::lock_guard lock{ GImpl->m_mutex };
        return GImpl->m_retiredGenerationCount;
    }
#endif


    void AssetManager::AddRequestRef(Internal::AssetAcquisition* request)
    {
        request->m_externalRefs.fetch_add(1);
    }


    void AssetManager::ReleaseRequest(Internal::AssetAcquisition* request)
    {
        FE_PROFILER_ZONE();

        if (!request->m_managerAlive.load(std::memory_order_acquire))
        {
            const uint32_t previous = request->m_externalRefs.fetch_sub(1);
            FE_Assert(previous > 0);
            if (previous == 1)
                Memory::DefaultDelete(request);
            return;
        }

        std::lock_guard lock{ GImpl->m_mutex };
        const uint32_t previous = request->m_externalRefs.fetch_sub(1);
        FE_Assert(previous > 0);
        if (previous != 1)
            return;

        if (request->m_result.load() == AssetLoadResult::kPending)
            GImpl->Finish(*request, AssetLoadResult::kCanceled, "asset request canceled");
        else
            GImpl->Release(*request);

        GImpl->DeleteIfUnused(request);
    }


    void AssetManager::CancelRequest(Internal::AssetAcquisition* request)
    {
        FE_PROFILER_ZONE();

        std::lock_guard lock{ GImpl->m_mutex };
        GImpl->Finish(*request, AssetLoadResult::kCanceled, "asset request canceled");
    }


    AssetRequest::AssetRequest(Internal::AssetAcquisition* request)
        : m_acquisition(request)
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
        if (this != &other)
        {
            Reset();

            m_acquisition = other.m_acquisition;
            if (m_acquisition)
                AssetManager::AddRequestRef(m_acquisition);
        }

        return *this;
    }


    AssetRequest& AssetRequest::operator=(AssetRequest&& other) noexcept
    {
        if (this != &other)
        {
            Reset();
            m_acquisition = other.m_acquisition;
            other.m_acquisition = nullptr;
        }

        return *this;
    }


    bool AssetRequest::IsValid() const
    {
        return m_acquisition && m_acquisition->m_root;
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


    AssetLoadResult AssetRequest::GetDiscoveryResult() const
    {
        return m_acquisition ? m_acquisition->m_discovery.load(std::memory_order_acquire) : AssetLoadResult::kFailed;
    }


    festd::string_view AssetRequest::GetError() const
    {
        if (!m_acquisition || m_acquisition->m_result.load(std::memory_order_acquire) == AssetLoadResult::kPending)
            return {};

        return m_acquisition->m_error;
    }


    AssetSlot* AssetRequest::GetAssetSlot() const
    {
        return m_acquisition ? m_acquisition->m_root : nullptr;
    }


    void AssetRequest::Cancel()
    {
        if (m_acquisition)
            AssetManager::CancelRequest(m_acquisition);
    }


    void AssetRequest::Wait() const
    {
        if (m_acquisition && !IsCompleted())
        {
            FE_Assert(!Threading::IsMainThread(), "AssetRequest::Wait cannot suspend a main-thread fiber");
            m_acquisition->m_completionEvent->Wait();
        }
    }


    void AssetRequest::WaitForDiscovery() const
    {
        if (m_acquisition && m_acquisition->m_discovery.load(std::memory_order_acquire) == AssetLoadResult::kPending)
            m_acquisition->m_discoveryEvent->Wait();
    }


    void AssetRequest::Reset()
    {
        if (m_acquisition)
        {
            AssetManager::ReleaseRequest(m_acquisition);
            m_acquisition = nullptr;
        }
    }
} // namespace FE::IO
