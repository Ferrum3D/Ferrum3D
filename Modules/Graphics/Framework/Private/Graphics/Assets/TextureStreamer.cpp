#include <Core/Threading/Thread.h>
#include <Graphics/Assets/TextureStreamer.h>
#include <Graphics/Assets/TextureStreamingOperation.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    namespace
    {
        template<class TEntry>
        TEntry* FindTextureEntry(festd::vector<TEntry>& entries, const void* asset)
        {
            for (TEntry& entry : entries)
            {
                if (entry.m_asset == asset)
                    return &entry;
            }
            return nullptr;
        }


        template<class TEntry>
        const TEntry* FindTextureEntry(const festd::vector<TEntry>& entries, const void* asset)
        {
            for (const TEntry& entry : entries)
            {
                if (entry.m_asset == asset)
                    return &entry;
            }
            return nullptr;
        }
    } // namespace


    struct TextureStreamer::Impl final
    {
        struct Entry final
        {
            TextureAsset* m_asset = nullptr;
            IO::ArtifactRecord m_artifact;
            uint32_t m_requestedMip = 0;
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
            TextureAsset& asset = *entry.m_asset;
            if (asset.m_residentMip == kInvalidIndex)
            {
                asset.m_currentOperation = CreateTextureStreamingOperation_CreateResource(asset,
                                                                                          entry.m_artifact,
                                                                                          entry.m_requestedMip,
                                                                                          m_device,
                                                                                          m_resourcePool,
                                                                                          m_asyncCopyQueue);
            }
            else if (entry.m_requestedMip > asset.m_residentMip)
            {
                asset.m_currentOperation = CreateTextureStreamingOperation_StreamIn(asset,
                                                                                    entry.m_artifact,
                                                                                    entry.m_requestedMip,
                                                                                    m_device,
                                                                                    m_resourcePool,
                                                                                    m_asyncCopyQueue);
            }
            else
            {
                asset.m_currentOperation = CreateTextureStreamingOperation_StreamOut(asset,
                                                                                     entry.m_artifact,
                                                                                     entry.m_requestedMip,
                                                                                     m_device,
                                                                                     m_resourcePool,
                                                                                     m_asyncCopyQueue);
            }
        }

        void Complete(Entry& entry)
        {
            TextureAsset& asset = *entry.m_asset;
            TextureStreamingOperation* operation = asset.m_currentOperation;
            asset.m_texture = operation->TakeTexture();
            asset.m_residentMip = operation->GetTargetIndex();
            operation->Destroy();
            asset.m_currentOperation = nullptr;
        }

        void Fail(Entry& entry)
        {
            TextureAsset& asset = *entry.m_asset;
            entry.m_failed = true;
            if (!entry.m_initializing)
                entry.m_requestedMip = asset.m_residentMip;
            asset.m_currentOperation->Destroy();
            asset.m_currentOperation = nullptr;
        }

        void Cancel(Entry& entry)
        {
            TextureStreamingOperation* operation = entry.m_asset->m_currentOperation;
            if (!operation)
                return;

            operation->Cancel();
            operation->Destroy();
            entry.m_asset->m_currentOperation = nullptr;
        }

        void Remove(TextureAsset* asset)
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


    TextureStreamer::TextureStreamer(Core::Device* device, Core::ResourcePool* resourcePool, Core::AsyncCopyQueue* asyncCopyQueue)
    {
        m_impl = Memory::DefaultNew<Impl>(device, resourcePool, asyncCopyQueue);
    }


    TextureStreamer::~TextureStreamer()
    {
        Memory::DefaultDelete(m_impl);
    }


    void TextureStreamer::SetResidentMip(const TextureAsset& asset, const uint32_t mipIndex)
    {
        FE_Assert(Threading::IsMainThread());
        Impl::Entry* entry = FindTextureEntry(m_impl->m_entries, &asset);
        FE_Assert(entry && mipIndex < asset.m_desc.m_mipSliceCount);
        const uint32_t minimumMip = asset.m_mipTailOffsets.empty() ? 0 : asset.m_mipTailOffsets.size() - 1;
        entry->m_requestedMip = Math::Max(mipIndex, minimumMip);
        entry->m_failed = false;
    }


    IO::AssetFinalizeResult TextureStreamer::FinalizeAssetLoading(IO::AssetSlot&, const IO::ArtifactRecord& artifact,
                                                                  void* candidate)
    {
        auto* asset = static_cast<TextureAsset*>(candidate);
        FE_Assert(!FindTextureEntry(m_impl->m_entries, asset));
        Impl::Entry& entry = m_impl->m_entries.emplace_back();
        entry.m_asset = asset;
        entry.m_artifact = artifact;
        entry.m_requestedMip = asset->m_mipTailOffsets.empty() ? 0 : asset->m_mipTailOffsets.size() - 1;
        return IO::AssetFinalizeResult::kPending;
    }


    IO::AssetFinalizeResult TextureStreamer::PollFinalize(IO::AssetSlot&, void* candidate)
    {
        Impl::Entry* entry = FindTextureEntry(m_impl->m_entries, candidate);
        if (!entry || entry->m_failed)
            return IO::AssetFinalizeResult::kFailed;
        if (entry->m_asset->m_currentOperation || !entry->m_asset->m_texture)
            return IO::AssetFinalizeResult::kPending;

        entry->m_initializing = false;
        return IO::AssetFinalizeResult::kSucceeded;
    }


    void TextureStreamer::CancelFinalize(IO::AssetSlot&, void* candidate)
    {
        m_impl->Remove(static_cast<TextureAsset*>(candidate));
    }


    void TextureStreamer::Tick()
    {
        for (Impl::Entry& entry : m_impl->m_entries)
        {
            TextureStreamingOperation* operation = entry.m_asset->m_currentOperation;
            if (operation)
            {
                const AssetStreamingOperationStatus status = operation->Tick();
                if (status == AssetStreamingOperationStatus::kSucceeded)
                    m_impl->Complete(entry);
                else if (status == AssetStreamingOperationStatus::kFailed)
                    m_impl->Fail(entry);
            }
            else if (!entry.m_failed && entry.m_asset->m_residentMip != entry.m_requestedMip)
            {
                m_impl->Start(entry);
            }
        }
    }


    bool TextureStreamer::HasRunningOperations(const IO::AssetSlot&, const void* asset) const
    {
        const Impl::Entry* entry = FindTextureEntry(m_impl->m_entries, asset);
        return entry && entry->m_asset->m_currentOperation;
    }


    void TextureStreamer::OnAssetDestroyed(IO::AssetSlot&, void* asset)
    {
        m_impl->Remove(static_cast<TextureAsset*>(asset));
    }


    void TextureStreamer::CancelAssetOperations(IO::AssetSlot&, void* asset)
    {
        if (Impl::Entry* entry = FindTextureEntry(m_impl->m_entries, asset))
            m_impl->Cancel(*entry);
    }
} // namespace FE::Graphics
