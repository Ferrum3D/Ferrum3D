#include <Core/Threading/Thread.h>
#include <Graphics/Assets/MeshStreamer.h>
#include <Graphics/Assets/MeshStreamingOperation.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    namespace
    {
        template<class TEntry>
        TEntry* FindMeshEntry(festd::vector<TEntry>& entries, const void* asset)
        {
            for (TEntry& entry : entries)
            {
                if (entry.m_asset == asset)
                    return &entry;
            }
            return nullptr;
        }


        template<class TEntry>
        const TEntry* FindMeshEntry(const festd::vector<TEntry>& entries, const void* asset)
        {
            for (const TEntry& entry : entries)
            {
                if (entry.m_asset == asset)
                    return &entry;
            }
            return nullptr;
        }
    } // namespace


    struct MeshStreamer::Impl final
    {
        struct Entry final
        {
            MeshAsset* m_asset = nullptr;
            IO::ArtifactRecord m_artifact;
            uint32_t m_requestedLod = 0;
            bool m_initializing = true;
            bool m_failed = false;
        };

        Impl(Core::Device* device, Core::ResourcePool* resourcePool, Core::AsyncCopyQueue* asyncCopyQueue)
            : m_device(device)
            , m_resourcePool(resourcePool)
            , m_asyncCopyQueue(asyncCopyQueue)
        {
            FE_Assert(device && resourcePool && asyncCopyQueue);
        }

        ~Impl()
        {
            while (!m_entries.empty())
                Remove(m_entries.back().m_asset);
        }

        void Start(Entry& entry)
        {
            MeshAsset& asset = *entry.m_asset;
            if (asset.m_residentLod == kInvalidIndex)
            {
                asset.m_currentOperation = CreateMeshStreamingOperation_CreateResource(asset,
                                                                                       entry.m_artifact,
                                                                                       entry.m_requestedLod,
                                                                                       m_device,
                                                                                       m_resourcePool,
                                                                                       m_asyncCopyQueue);
            }
            else if (entry.m_requestedLod > asset.m_residentLod)
            {
                asset.m_currentOperation = CreateMeshStreamingOperation_StreamIn(asset,
                                                                                 entry.m_artifact,
                                                                                 entry.m_requestedLod,
                                                                                 m_device,
                                                                                 m_resourcePool,
                                                                                 m_asyncCopyQueue);
            }
            else
            {
                asset.m_currentOperation = CreateMeshStreamingOperation_StreamOut(asset,
                                                                                  entry.m_artifact,
                                                                                  entry.m_requestedLod,
                                                                                  m_device,
                                                                                  m_resourcePool,
                                                                                  m_asyncCopyQueue);
            }
        }

        void Complete(Entry& entry)
        {
            MeshAsset& asset = *entry.m_asset;
            MeshStreamingOperation* operation = asset.m_currentOperation;
            asset.m_buffer = operation->TakeBuffer();
            asset.m_residentLod = operation->GetTargetIndex();
            operation->Destroy();
            asset.m_currentOperation = nullptr;
        }

        void Fail(Entry& entry)
        {
            MeshAsset& asset = *entry.m_asset;
            entry.m_failed = true;
            if (!entry.m_initializing)
                entry.m_requestedLod = asset.m_residentLod;
            asset.m_currentOperation->Destroy();
            asset.m_currentOperation = nullptr;
        }

        void Cancel(Entry& entry)
        {
            MeshStreamingOperation* operation = entry.m_asset->m_currentOperation;
            if (!operation)
                return;

            operation->Cancel();
            operation->Destroy();
            entry.m_asset->m_currentOperation = nullptr;
        }

        void Remove(MeshAsset* asset)
        {
            for (uint32_t index = 0; index < m_entries.size(); ++index)
            {
                if (m_entries[index].m_asset != asset)
                    continue;

                Cancel(m_entries[index]);
                m_entries[index] = std::move(m_entries.back());
                m_entries.pop_back();
                return;
            }
        }

        Core::Device* m_device = nullptr;
        Core::ResourcePool* m_resourcePool = nullptr;
        Core::AsyncCopyQueue* m_asyncCopyQueue = nullptr;
        festd::vector<Entry> m_entries;
    };


    MeshStreamer::MeshStreamer(Core::Device* device, Core::ResourcePool* resourcePool, Core::AsyncCopyQueue* asyncCopyQueue)
    {
        m_impl = Memory::DefaultNew<Impl>(device, resourcePool, asyncCopyQueue);
    }


    MeshStreamer::~MeshStreamer()
    {
        Memory::DefaultDelete(m_impl);
    }


    void MeshStreamer::SetResidentLod(MeshAsset& asset, const uint32_t lodIndex)
    {
        FE_Assert(Threading::IsMainThread());
        Impl::Entry* entry = FindMeshEntry(m_impl->m_entries, &asset);
        FE_Assert(entry && lodIndex < asset.m_lodErrors.size());
        entry->m_requestedLod = lodIndex;
        entry->m_failed = false;
    }


    IO::AssetFinalizeResult MeshStreamer::FinalizeAssetLoading(IO::AssetSlot&, const IO::ArtifactRecord& artifact,
                                                               void* candidate)
    {
        auto* asset = static_cast<MeshAsset*>(candidate);
        FE_Assert(!FindMeshEntry(m_impl->m_entries, asset));
        Impl::Entry& entry = m_impl->m_entries.emplace_back();
        entry.m_asset = asset;
        entry.m_artifact = artifact;
        entry.m_requestedLod = 0;
        return IO::AssetFinalizeResult::kPending;
    }


    IO::AssetFinalizeResult MeshStreamer::PollFinalize(IO::AssetSlot&, void* candidate)
    {
        Impl::Entry* entry = FindMeshEntry(m_impl->m_entries, candidate);
        if (!entry || entry->m_failed)
            return IO::AssetFinalizeResult::kFailed;
        if (entry->m_asset->m_currentOperation || !entry->m_asset->m_buffer)
            return IO::AssetFinalizeResult::kPending;

        entry->m_initializing = false;
        return IO::AssetFinalizeResult::kSucceeded;
    }


    void MeshStreamer::CancelFinalize(IO::AssetSlot&, void* candidate)
    {
        m_impl->Remove(static_cast<MeshAsset*>(candidate));
    }


    void MeshStreamer::Tick()
    {
        for (Impl::Entry& entry : m_impl->m_entries)
        {
            MeshStreamingOperation* operation = entry.m_asset->m_currentOperation;
            if (operation)
            {
                const AssetStreamingOperationStatus status = operation->Tick();
                if (status == AssetStreamingOperationStatus::kSucceeded)
                    m_impl->Complete(entry);
                else if (status == AssetStreamingOperationStatus::kFailed)
                    m_impl->Fail(entry);
            }
            else if (!entry.m_failed && entry.m_asset->m_residentLod != entry.m_requestedLod)
            {
                m_impl->Start(entry);
            }
        }
    }


    bool MeshStreamer::HasRunningOperations(const IO::AssetSlot&, const void* asset) const
    {
        const Impl::Entry* entry = FindMeshEntry(m_impl->m_entries, asset);
        return entry && entry->m_asset->m_currentOperation;
    }


    void MeshStreamer::OnAssetDestroyed(IO::AssetSlot&, void* asset)
    {
        m_impl->Remove(static_cast<MeshAsset*>(asset));
    }


    void MeshStreamer::CancelAssetOperations(IO::AssetSlot&, void* asset)
    {
        if (Impl::Entry* entry = FindMeshEntry(m_impl->m_entries, asset))
            m_impl->Cancel(*entry);
    }
} // namespace FE::Graphics
