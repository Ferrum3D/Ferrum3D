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
#include <cctype>
#include <festd/unordered_map.h>
#include <festd/vector.h>

namespace FE::IO
{
    namespace Internal
    {
        struct AssetAcquisition final
        {
            std::atomic<uint32_t> m_externalRefs = 1;
            uint32_t m_workRefs = 0;
            AssetSlot* m_root = nullptr;
            festd::unordered_dense_map<AssetID, Rtti::TypeID> m_members;
            festd::vector<AssetSlot*> m_residentSlots;
            uint32_t m_pendingMetadata = 0;
            bool m_released = false;
            std::atomic<AssetLoadResult> m_discovery = AssetLoadResult::kPending;
            std::atomic<AssetLoadResult> m_result = AssetLoadResult::kPending;
            Rc<WaitGroup> m_discoveryEvent = WaitGroup::Create();
            Rc<WaitGroup> m_completionEvent = WaitGroup::Create();
            festd::inline_string m_error;
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
            // A terminal error occurred before publication. No candidate from this operation is exposed.
            kFailed
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
            AssetFinalizeResult m_finalize = AssetFinalizeResult::kPending;
            uint32_t m_readCount = 1;
        };


        struct BindingState
        {
            void* m_context;
            Operation* m_owner;
            AssetSlot* (*m_resolve)(void*, Operation&, AssetID, Rtti::TypeID, DependencyKind);
        };
        thread_local BindingState* GBinding = nullptr;


        void DestroyObject(Operation& operation, void* object)
        {
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
        Threading::Mutex m_mutex;
        festd::unordered_dense_map<AssetID, AssetSlot*> m_slots;
        festd::unordered_dense_map<AssetID, Operation*> m_operations;
        festd::unordered_dense_map<Rtti::TypeID, Streamer*> m_streamers;
        festd::vector<AssetSlot*> m_ownedSlots;
        festd::vector<Operation*> m_ownedOperations;
        festd::vector<AssetPublicationGate*> m_gates;
        festd::vector<Rc<WaitGroup>> m_jobs;
        festd::vector<Internal::AssetAcquisition*> m_waiters;
        ConcurrentOnceConsumedQueue m_payloadCompletions;
        DefaultStreamer m_defaultStreamer;
        bool m_shuttingDown = false;
        uint32_t m_nextGroup = 0;
#if FE_DEVELOPMENT
        Rc<WaitGroup> m_entered;
        Rc<WaitGroup> m_resume;
#endif

        ~Impl();
        AssetSlot& Slot(AssetID id);
        Operation& GetOperation(AssetID id, festd::vector<Operation*>& starts);
        void Join(Internal::AssetAcquisition& request, AssetID id, Rtti::TypeID type, festd::vector<Operation*>& starts);
        void Expand(Internal::AssetAcquisition& request, Operation& operation, festd::vector<Operation*>& starts);
        void CompleteMetadata(Operation& operation, ArtifactDecodeResult&& result);
        void StartMetadata(Operation& operation);
        void StartPayload(Operation& operation);
        void Release(Internal::AssetAcquisition& request);
        void Finish(Internal::AssetAcquisition& request, AssetLoadResult result, festd::string_view error = {});
        void DiscoveryDone(Internal::AssetAcquisition& request);
        void DetectGroups(Internal::AssetAcquisition& request);
        void DeleteIfUnused(Internal::AssetAcquisition* request);
        Streamer& GetStreamer(Operation& operation);
        AssetSlot* Resolve(Operation& owner, AssetID id, Rtti::TypeID type, DependencyKind kind);
        void Tick();
    };

    AssetManager::Impl* AssetManager::GImpl = nullptr;


    AssetManager::Impl::~Impl()
    {
        for (Operation* operation : m_ownedOperations)
        {
            void* object =
                operation->m_state == State::kPublished ? operation->m_slot->m_instance.load() : operation->m_candidate;
            DestroyObject(*operation, object);
            Memory::DefaultDelete(operation);
        }

        for (AssetPublicationGate* gate : m_gates)
            Memory::DefaultDelete(gate);
        for (AssetSlot* slot : m_ownedSlots)
            Memory::DefaultDelete(slot);
    }


    AssetSlot& AssetManager::Impl::Slot(AssetID id)
    {
        auto found = m_slots.find(id);
        if (found != m_slots.end())
        {
            return *found->second;
        }
        AssetSlot* slot = Memory::DefaultNew<AssetSlot>();
        slot->m_assetId = id;
        m_slots.insert({ id, slot });
        m_ownedSlots.push_back(slot);
        return *slot;
    }


    Operation& AssetManager::Impl::GetOperation(AssetID id, festd::vector<Operation*>& starts)
    {
        auto found = m_operations.find(id);
        if (found != m_operations.end())
            return *found->second;

        Operation* operation = Memory::DefaultNew<Operation>();
        operation->m_id = id;
        operation->m_slot = &Slot(id);
        m_operations.insert({ id, operation });
        m_ownedOperations.push_back(operation);
        starts.push_back(operation);
        return *operation;
    }


    void AssetManager::Impl::Join(Internal::AssetAcquisition& request, AssetID id, Rtti::TypeID type,
                                  festd::vector<Operation*>& starts)
    {
        if (request.m_discovery.load() != AssetLoadResult::kPending)
            return;

        auto member = request.m_members.find(id);
        if (member != request.m_members.end())
        {
            if (type.IsValid() && member->second.IsValid() && member->second != type)
                Finish(request, AssetLoadResult::kFailed, "incompatible dependency types");
            else if (type.IsValid())
                member->second = type;

            return;
        }

        // Discovery is acquisition-local, while the underlying operation is shared. Recording membership before expanding the
        // operation prevents cycles and repeated dependencies from contributing residency more than once to this acquisition.
        Operation& operation = GetOperation(id, starts);
        request.m_members.insert({ id, type });
        request.m_residentSlots.push_back(operation.m_slot);
        operation.m_slot->m_strongRefCount.fetch_add(1);
        if (operation.m_state == State::kFailed)
        {
            Finish(request, AssetLoadResult::kFailed, operation.m_error);
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
        Rtti::TypeID expected = request.m_members.find(operation.m_id)->second;
        if (expected.IsValid() && expected != operation.m_record.m_assetTypeId)
        {
            Finish(request, AssetLoadResult::kFailed, "dependency type does not match metadata");
            return;
        }

        // Only hard dependencies extend the load closure. Soft and optional links are validated and may bind to an existing slot,
        // but deserialization never starts additional discovery for them.
        for (const ArtifactDependencyRecord& dependency : operation.m_record.m_dependencies)
        {
            if (dependency.m_kind == DependencyKind::kHard)
                Join(request, dependency.m_assetId, dependency.m_expectedTypeId, starts);
        }
    }


    void AssetManager::Impl::CompleteMetadata(Operation& operation, ArtifactDecodeResult&& result)
    {
        festd::vector<Operation*> starts;
        bool payload = false;
        {
            std::lock_guard lock{ m_mutex };
            if (result)
            {
                operation.m_record = std::move(result.value());
                operation.m_type = Rtti::TypeRegistry::FindType(operation.m_record.m_assetTypeId);
                operation.m_slot->m_typeId = operation.m_record.m_assetTypeId;
                operation.m_slot->m_currentArtifactId = operation.m_record.m_artifactId;
                operation.m_state = State::kPayload;
                payload = true;
            }
            else
            {
                operation.m_error = result.error().m_message;
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
                        Finish(*request, AssetLoadResult::kFailed, operation.m_error);
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
        // Pipeline stage 1: resolve and decode artifact metadata on a worker. CompleteMetadata performs recursive hard-dependency
        // discovery and starts the payload stage once this operation has a valid record.
        Rc<WaitGroup> job = WaitGroup::Create();
        Jobs::DispatchBackground(
            [this, &operation] {
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
                ResolvedDataSource source = ArtifactStore::ResolveMeta(operation.m_id);
                festd::pmr::vector<std::byte> bytes;
                Rc<WaitGroup> done = WaitGroup::Create();
                Async::Batch batch(source, done.Get());
                batch.ReadAppendToEnd(bytes);
                Rc<Async::IController> controller = Async::Read(batch);
                done->Wait();
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

        std::lock_guard lock{ m_mutex };
        m_jobs.push_back(std::move(job));
    }


    void AssetManager::Impl::StartPayload(Operation& operation)
    {
        // Pipeline stage 2: read all planned chunks, decompress them into one logical payload, validate every decoded chunk, and
        // deserialize a private candidate. Publication is deliberately deferred to Tick.
        Rc<WaitGroup> job = WaitGroup::Create();
        Jobs::DispatchBackground(
            [this, &operation] {
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
                done->Wait();

                if (controller->GetStatus() != Async::Status::kSucceeded)
                    operation.m_error = "primary payload read failed";

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

                if (operation.m_error.empty() && (!operation.m_type || !operation.m_type->m_deserialize))
                {
                    operation.m_error = "asset type has no RTTI deserializer";
                }

                if (operation.m_error.empty())
                {
                    operation.m_candidate = Memory::DefaultAllocate(operation.m_type->m_size, operation.m_type->m_alignment);
                    if (operation.m_type->m_defaultConstructor)
                        operation.m_type->m_defaultConstructor(operation.m_candidate);
                    else
                        memset(operation.m_candidate, 0, operation.m_type->m_size);

                    ReadOnlyMemoryStream stream(bytes);
                    BindingState binding{
                        this,
                        &operation,
                        [](void* context, Operation& owner, AssetID id, Rtti::TypeID type, DependencyKind kind) {
                            return static_cast<Impl*>(context)->Resolve(owner, id, type, kind);
                        }
                    };

                    // Link<T> deserialization consults this thread-local scope. Resolve accepts only links declared by the
                    // candidate's metadata and returns already reserved slots; it cannot discover assets or initiate I/O.
                    GBinding = &binding;
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

                    GBinding = nullptr;
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

        std::lock_guard lock{ m_mutex };
        m_jobs.push_back(std::move(job));
    }


    void AssetManager::Impl::Release(Internal::AssetAcquisition& request)
    {
        if (request.m_released)
            return;

        request.m_released = true;
        for (AssetSlot* slot : request.m_residentSlots)
        {
            uint32_t previous = slot->m_strongRefCount.fetch_sub(1);
            FE_Assert(previous > 0, "Asset residency count underflow");
        }
    }


    void AssetManager::Impl::Finish(Internal::AssetAcquisition& request, AssetLoadResult result, festd::string_view error)
    {
        if (request.m_result.load() != AssetLoadResult::kPending)
            return;

        if (!error.empty())
            request.m_error.assign(error.data(), error.size());

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
            nodes.push_back({ m_operations.find(member.first)->second });
        }

        festd::vector<uint32_t> stack;
        uint32_t next = 0;
        const auto visit = [&](auto&& self, uint32_t index) -> void {
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

            uint32_t group = kInvalidIndex;
            for (uint32_t member : component)
            {
                uint32_t existing = nodes[member].m_operation->m_slot->m_referenceGroup.load();
                if (existing != kInvalidIndex)
                    group = existing;
            }

            if (group == kInvalidIndex)
                group = m_nextGroup++;

            for (uint32_t member : component)
                nodes[member].m_operation->m_slot->m_referenceGroup.store(group);
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


    AssetSlot* AssetManager::Impl::Resolve(Operation& owner, AssetID id, Rtti::TypeID type, DependencyKind kind)
    {
        if (!id.IsValid())
            return nullptr;

        // Serialized data is not allowed to introduce dependencies absent from trusted metadata or to change their expected type
        // or loading semantics. Returning an existing stable slot also ensures binding itself has no I/O side effects.
        bool declared = false;
        for (const ArtifactDependencyRecord& dependency : owner.m_record.m_dependencies)
            declared |= dependency.m_assetId == id && dependency.m_expectedTypeId == type && dependency.m_kind == kind;

        if (!declared)
            return nullptr;

        std::lock_guard lock{ m_mutex };
        auto found = m_slots.find(id);
        return found == m_slots.end() ? nullptr : found->second;
    }


    void AssetManager::Impl::Tick()
    {
        // Pipeline stage 3: transfer background deserialization results to the main-thread state machine. Workers only enqueue
        // completions, so streamer callbacks and publication always happen from Tick.
        ConcurrentOnceConsumedQueue::Node* completion = m_payloadCompletions.DequeueAll();
        while (completion)
        {
            Operation* operation = static_cast<Operation*>(completion);
            completion = completion->m_next;
            operation->m_state = operation->m_error.empty() ? State::kCandidate : State::kFailed;
        }

        bool progressed = true;
        festd::unordered_dense_set<uint32_t> processedGroups;
        festd::unordered_dense_set<Operation*> processedSingles;
        while (progressed)
        {
            progressed = false;
            for (Operation* seed : m_ownedOperations)
            {
                if (seed->m_state != State::kCandidate && seed->m_state != State::kFinalizing)
                    continue;

                uint32_t group = seed->m_slot->m_referenceGroup.load();
                if (group == kInvalidIndex)
                {
                    if (!processedSingles.insert(seed).second)
                        continue;
                }
                else if (!processedGroups.insert(group).second)
                {
                    continue;
                }

                festd::inline_vector<Operation*, 8> members;
                if (group == kInvalidIndex)
                {
                    members.push_back(seed);
                }
                else
                {
                    for (Operation* operation : m_ownedOperations)
                    {
                        if (operation->m_slot->m_referenceGroup.load() == group)
                            members.push_back(operation);
                    }
                }

                // A reference group advances as a unit. In particular, a hard cycle cannot finalize or publish while one of its
                // candidates is still being produced by a worker.
                bool candidatesReady = true;
                bool orderedCycle = false;
                for (Operation* member : members)
                {
                    candidatesReady &= member->m_state == State::kCandidate || member->m_state == State::kFinalizing
                        || member->m_state == State::kPublished;
                    orderedCycle |= members.size() > 1 && GetStreamer(*member).RequiresFinalizedDependencies();
                }

                if (!candidatesReady)
                    continue;

                // A streamer that needs already-finalized dependencies imposes an ordering edge. Such an edge cannot be satisfied
                // inside a cycle, so fail deterministically instead of leaving the group pending forever.
                if (orderedCycle)
                {
                    for (Operation* member : members)
                    {
                        member->m_error = "finalization-order cycle is unsupported";
                        member->m_state = State::kFailed;
                        DestroyObject(*member, member->m_candidate);
                        member->m_candidate = nullptr;
                    }

                    progressed = true;
                    continue;
                }

                // Dependencies inside this group are candidates by construction. Hard dependencies outside the group must already
                // be published before finalization starts, which gives acyclic graphs dependency-first ordering.
                bool dependenciesReady = true;
                for (Operation* member : members)
                {
                    for (const ArtifactDependencyRecord& dependency : member->m_record.m_dependencies)
                    {
                        if (dependency.m_kind != DependencyKind::kHard)
                            continue;

                        Operation* target = m_operations.find(dependency.m_assetId)->second;
                        bool sameGroup = group != kInvalidIndex && target->m_slot->m_referenceGroup.load() == group;
                        if (!sameGroup && target->m_state != State::kPublished)
                            dependenciesReady = false;
                    }
                }

                if (!dependenciesReady)
                    continue;

                // Pipeline stage 4: start or poll type-specific finalization. Pending finalizers retain their private candidate and
                // are revisited by a later Tick; failure destroys every unpublished member of the group.
                bool finalized = true;
                bool failed = false;
                for (Operation* member : members)
                {
                    if (member->m_state == State::kPublished)
                        continue;

                    Streamer& streamer = GetStreamer(*member);
                    if (member->m_state == State::kCandidate)
                    {
                        member->m_finalize = streamer.FinalizeAssetLoading(*member->m_slot, member->m_candidate);
                        member->m_state = State::kFinalizing;
                    }
                    else if (member->m_finalize == AssetFinalizeResult::kPending)
                    {
                        member->m_finalize = streamer.PollFinalize(*member->m_slot, member->m_candidate);
                    }

                    finalized &= member->m_finalize == AssetFinalizeResult::kSucceeded;
                    failed |= member->m_finalize == AssetFinalizeResult::kFailed;
                }

                if (failed)
                {
                    for (Operation* member : members)
                    {
                        if (member->m_state != State::kPublished)
                        {
                            member->m_error = "asset finalization failed";
                            member->m_state = State::kFailed;
                            DestroyObject(*member, member->m_candidate);
                            member->m_candidate = nullptr;
                        }
                    }

                    progressed = true;
                    continue;
                }

                if (!finalized)
                    continue;

                // Pipeline stage 5: commit the group. Slot contents are installed while the shared gate is closed. The release
                // store opens the gate only after every member is installed, giving readers one visibility boundary for the group.
                AssetPublicationGate* gate = Memory::DefaultNew<AssetPublicationGate>();
                m_gates.push_back(gate);
                for (Operation* member : members)
                {
                    if (member->m_state != State::kPublished)
                    {
                        member->m_slot->m_publicationGate.store(gate, std::memory_order_relaxed);
                        member->m_slot->m_instance.store(member->m_candidate, std::memory_order_relaxed);
                        member->m_candidate = nullptr;
                        member->m_slot->m_generation.fetch_add(1, std::memory_order_relaxed);
                        member->m_slot->m_completed.store(true, std::memory_order_relaxed);
                        member->m_state = State::kPublished;
                    }
                }

                gate->m_isOpen.store(true, std::memory_order_release);
                progressed = true;
            }
        }

        // Complete acquisitions only after every member of their hard closure is published, or as soon as any member fails.
        for (uint32_t index = 0; index < m_waiters.size();)
        {
            Internal::AssetAcquisition* request = m_waiters[index];

            bool failed = false, published = true;
            festd::string_view error;
            for (const auto& member : request->m_members)
            {
                Operation* operation = m_operations.find(member.first)->second;
                failed |= operation->m_state == State::kFailed;
                published &= operation->m_state == State::kPublished;
                if (operation->m_state == State::kFailed)
                    error = operation->m_error;
            }

            if (!failed && !published)
            {
                ++index;
                continue;
            }

            Finish(*request, failed ? AssetLoadResult::kFailed : AssetLoadResult::kSucceeded, error);
            --request->m_workRefs;
            m_waiters.erase(m_waiters.begin() + index);
            DeleteIfUnused(request);
        }
    }


    void AssetManager::Impl::DeleteIfUnused(Internal::AssetAcquisition* request)
    {
        if (request->m_externalRefs.load() == 0 && request->m_workRefs == 0)
            Memory::DefaultDelete(request);
    }


    namespace Internal
    {
        AssetSlot* ResolveAssetLink(AssetID id, Rtti::TypeID type, DependencyKind kind)
        {
            if (!GBinding)
            {
                return nullptr;
            }
            return GBinding->m_resolve(GBinding->m_context, *GBinding->m_owner, id, type, kind);
        }


        bool IsAssetBindingActive()
        {
            return GBinding != nullptr;
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


    void AssetManager::Init()
    {
        FE_Assert(!GImpl);
        GImpl = Memory::DefaultNew<Impl>();
    }


    void AssetManager::Shutdown()
    {
        FE_Assert(GImpl);
        {
            std::lock_guard lock{ GImpl->m_mutex };
            GImpl->m_shuttingDown = true;
        }

        uint32_t waitedCount = 0;
        for (;;)
        {
            festd::vector<Rc<WaitGroup>> jobs;
            {
                std::lock_guard lock{ GImpl->m_mutex };
                jobs = GImpl->m_jobs;
            }
            WaitGroup::WaitAll(jobs);
            waitedCount = jobs.size();
            std::lock_guard lock{ GImpl->m_mutex };
            if (GImpl->m_jobs.size() == waitedCount)
            {
                break;
            }
        }
        GImpl->Tick();
        Memory::DefaultDelete(GImpl);
        GImpl = nullptr;
    }


    AssetRequest AssetManager::LoadAsset(AssetID id)
    {
        FE_Assert(GImpl);
        if (!id.IsValid())
            return {};

        auto* request = Memory::DefaultNew<Internal::AssetAcquisition>();
        festd::vector<Operation*> starts;
        {
            std::lock_guard lock{ GImpl->m_mutex };
            FE_Assert(!GImpl->m_shuttingDown);
            GImpl->Join(*request, id, Rtti::TypeID::kNull, starts);
            request->m_root = &GImpl->Slot(id);
            if (request->m_pendingMetadata == 0 && request->m_discovery.load() == AssetLoadResult::kPending)
                GImpl->DiscoveryDone(*request);
        }

        for (Operation* operation : starts)
            GImpl->StartMetadata(*operation);

        return AssetRequest(request);
    }


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
        auto found = GImpl->m_operations.find(id);
        return found == GImpl->m_operations.end() ? 0 : found->second->m_readCount;
    }
#endif


    void AssetManager::AddRequestRef(Internal::AssetAcquisition* request)
    {
        request->m_externalRefs.fetch_add(1);
    }


    void AssetManager::ReleaseRequest(Internal::AssetAcquisition* request)
    {
        uint32_t previous = request->m_externalRefs.fetch_sub(1);
        FE_Assert(previous > 0);
        if (previous != 1)
            return;

        std::lock_guard lock{ GImpl->m_mutex };
        if (request->m_result.load() == AssetLoadResult::kPending)
            GImpl->Finish(*request, AssetLoadResult::kCanceled, "asset request canceled");
        else
            GImpl->Release(*request);

        GImpl->DeleteIfUnused(request);
    }


    void AssetManager::CancelRequest(Internal::AssetAcquisition* request)
    {
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


    AssetLoadResult AssetRequest::GetDiscoveryResult() const
    {
        return m_acquisition ? m_acquisition->m_discovery.load(std::memory_order_acquire) : AssetLoadResult::kFailed;
    }


    festd::string_view AssetRequest::GetError() const
    {
        return m_acquisition ? festd::string_view(m_acquisition->m_error) : festd::string_view{};
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
            m_acquisition->m_completionEvent->Wait();
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
