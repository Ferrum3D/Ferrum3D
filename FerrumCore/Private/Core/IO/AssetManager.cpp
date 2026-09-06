#include <Core/Containers/SegmentedVector.h>
#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/IO/Async.h>
#include <Core/IO/StreamBase.h>
#include <Core/Jobs/JobGraph.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Serialization/Serialization.h>
#include <festd/bit_vector.h>
#include <festd/unordered_map.h>

namespace FE::IO
{
    struct AssetSlotInternal final
    {
        uint32_t m_index = kInvalidIndex;
        AssetSlot m_slot;

        ArtifactRecord m_artifactRecord;
        festd::pmr::vector<std::byte> m_readBuffer;
    };


    struct AssetManager::Impl final
    {
        festd::unordered_dense_map<Rtti::TypeID, Streamer*> m_assetTypeToStreamerMap;
        DefaultStreamer m_defaultStreamer;

        festd::unordered_dense_map<AssetID, uint32_t> m_assetIdToSlotMap;
        SegmentedVector<AssetSlotInternal> m_assetSlots;
        festd::bit_vector m_freeAssetSlots;
        festd::bit_vector m_initialReadCompleted;
        festd::bit_vector m_pendingFinalize;
        festd::bit_vector m_liveAssetSlots;

        Streamer& GetStreamer(Rtti::TypeID assetTypeId);
        AssetSlotInternal& AllocateAssetSlot(AssetID assetId);
    };

    AssetManager::Impl* AssetManager::GImpl = nullptr;


    Streamer& AssetManager::Impl::GetStreamer(const Rtti::TypeID assetTypeId)
    {
        const auto it = m_assetTypeToStreamerMap.find(assetTypeId);
        if (it == m_assetTypeToStreamerMap.end())
            return m_defaultStreamer;

        return *it->second;
    }


    AssetSlotInternal& AssetManager::Impl::AllocateAssetSlot(const AssetID assetId)
    {
        uint32_t slotIndex = m_freeAssetSlots.find_first();
        if (slotIndex == kInvalidIndex)
        {
            constexpr uint32_t kGrowSize = 256;
            m_freeAssetSlots.resize(m_freeAssetSlots.size() + kGrowSize, true);
            m_initialReadCompleted.resize(m_initialReadCompleted.size() + kGrowSize, false);
            m_pendingFinalize.resize(m_pendingFinalize.size() + kGrowSize, false);
            m_liveAssetSlots.resize(m_liveAssetSlots.size() + kGrowSize, false);
            for (uint32_t index = 0; index < kGrowSize; ++index)
                m_assetSlots.emplace_back();
            slotIndex = m_freeAssetSlots.find_first();
        }

        m_freeAssetSlots.reset(slotIndex);
        m_assetIdToSlotMap.insert({ assetId, slotIndex });
        AssetSlotInternal& slot = m_assetSlots[slotIndex];
        slot.m_index = slotIndex;
        slot.m_slot.m_assetId = assetId;
        slot.m_slot.m_typeId = Rtti::TypeID::kNull;
        slot.m_slot.m_currentArtifactId = ArtifactID::kNull;
        return slot;
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
        FE_Assert(GImpl == nullptr, "Asset Manager already initialized");
        GImpl = Memory::DefaultNew<Impl>();
    }


    void AssetManager::Shutdown()
    {
        FE_Assert(GImpl != nullptr, "Asset Manager not initialized");
        Memory::DefaultDelete(GImpl);
        GImpl = nullptr;
    }


    ResidencyTicket AssetManager::LoadAsset(const AssetID assetId)
    {
        // TODO: this function could be called from any thread, so proper synchronization is necessary.

        auto it = GImpl->m_assetIdToSlotMap.find(assetId);
        if (it != GImpl->m_assetIdToSlotMap.end())
            return ResidencyTicket{ &GImpl->m_assetSlots[it->second].m_slot };

        const ResolvedDataSource artifactMetaLocation = ArtifactStore::ResolveMeta(assetId);
        if (!artifactMetaLocation.IsValid())
            return ResidencyTicket{ nullptr };

        AssetSlotInternal& slot = GImpl->AllocateAssetSlot(assetId);

        Rc<WaitGroup> completionWaitGroup = WaitGroup::Create();
        Async::Batch artifactMetaBatch(artifactMetaLocation, completionWaitGroup.Get());
        artifactMetaBatch.ReadAppendToEnd(slot.m_readBuffer);
        slot.m_slot.m_asyncController = Async::Read(artifactMetaBatch);

        Jobs::Graph graph("LoadAsset", Jobs::FiberAffinityMask::kAllBackground);
        completionWaitGroup = graph.Dispatch("DeserializeArtifact", { completionWaitGroup }, [&slot] {
            ReadOnlyMemoryStream sourceData(slot.m_readBuffer);
            Serialization::TaggedBinaryFormat format;
            Serialization::DeserializationContext context(&sourceData, format);

            const Serialization::ResultCode serializationResult = context.Load(slot.m_artifactRecord);
            FE_Assert(serializationResult == Serialization::ResultCode::kSuccess); // TODO: error handling

            for (const AssetID& dependency : slot.m_artifactRecord.m_dependencies)
            {
                // TODO: figure out how to handle dependency loading and ref counting.
                static_cast<void>(LoadAsset(dependency));
            }
        });
        graph.Dispatch("InitialRead", { completionWaitGroup }, [&slot] {
            const ArtifactPayloadRecord& payloadRecord = slot.m_artifactRecord.m_payloads[0];

            // TODO: read the first payload, it must contain the actual asset data.
            //       The rest of the payloads contain LODs if required for asset type, and
            //       and are handled by corresponding streamers.

            slot.m_readBuffer.clear();

            uint32_t totalPayloadSize = 0;
            for (const ArtifactChunkRecord& chunkRecord : payloadRecord.m_chunks)
                totalPayloadSize += static_cast<uint32_t>(chunkRecord.m_uncompressedSize);
            slot.m_readBuffer.reserve(totalPayloadSize);

            Rc<WaitGroup> initialReadCompleted = WaitGroup::Create();
            Async::Batch initialReadBatch(payloadRecord.m_resolvedDataSource, initialReadCompleted.Get());
            for (const ArtifactChunkRecord& chunkRecord : payloadRecord.m_chunks)
                initialReadBatch.ReadAppend(slot.m_readBuffer, chunkRecord.m_uncompressedSize, chunkRecord.m_offsetInPayload);

            Async::Read(initialReadBatch); // TODO: where to store the controller and how to handle cancellation?

            // We can take advantage of our fiber-based job system here, and just suspend the current fiber until
            // async I/O batch is completed.
            initialReadCompleted->Wait();

            // TODO: deserialize the asset and notify the streamer.
        });

        graph.Detach();

        return ResidencyTicket{ &slot.m_slot };
    }


    AssetSlot* AssetManager::FindAssetSlot(const AssetID assetId)
    {
        const auto it = GImpl->m_assetIdToSlotMap.find(assetId);
        if (it == GImpl->m_assetIdToSlotMap.end())
            return nullptr;

        return &GImpl->m_assetSlots[it->second].m_slot;
    }


    void AssetManager::Tick()
    {
        Bit::Traverse(GImpl->m_initialReadCompleted.view(), [](const uint32_t assetIndex) {
            AssetSlot& slot = GImpl->m_assetSlots[assetIndex].m_slot;
            Streamer& streamer = GImpl->GetStreamer(slot.m_typeId);

            const bool finalizeCompleted = streamer.FinalizeAssetLoading(slot);
            GImpl->m_pendingFinalize.set(assetIndex, !finalizeCompleted);
            GImpl->m_liveAssetSlots.set(assetIndex, finalizeCompleted);
        });
        GImpl->m_initialReadCompleted.reset();
    }
} // namespace FE::IO
