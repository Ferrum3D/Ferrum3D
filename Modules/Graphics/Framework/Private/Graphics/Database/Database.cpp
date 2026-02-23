#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Core/Fence.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Database/Database.h>

namespace FE::Graphics::DB
{
    namespace
    {
        Memory::SpinLockedPool<StoragePage> GStoragePagePool{ "Graphics/DB/StoragePagePool", 2 * 1024 * 1024 };
    }


    void StoragePage::Update(const uint32_t byteOffset, const festd::span<const std::byte> data)
    {
        memcpy(m_hostStorage + byteOffset, data.data(), data.size());
    }


    StoragePage* StoragePage::Allocate(Core::ResourcePool* resourcePool)
    {
        StoragePage* page = GStoragePagePool.New();
        page->m_deviceStorage = resourcePool->CreateByteAddressBuffer("StoragePage", kTablePageSize);

        Core::BufferCommitParams commitParams;
        commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
        commitParams.m_bindFlags = Core::BarrierAccessFlags::kCopySource | Core::BarrierAccessFlags::kCopyDest
            | Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite;
        resourcePool->CommitBufferMemory(page->m_deviceStorage.Get(), commitParams);

        return page;
    }


    PageReplicationManager::PageReplicationManager(Core::ResourcePool* resourcePool)
        : m_resourcePool(resourcePool)
    {
    }


    void PageReplicationManager::UploadPage(Core::FrameGraph& graph, const StoragePage* page)
    {
        FE_Assert(page->m_replicationPolicy == PageReplicationPolicy::kCopyAllData, "Not implemented");

        CheckPendingPages();

        const Rc stagingBuffer = m_resourcePool->CreateByteAddressBuffer("PageReplicationManagerStaging", kTablePageSize);
        m_resourcePool->CommitBufferMemory(stagingBuffer.Get(),
                                           { Core::BarrierAccessFlags::kCopySource | Core::BarrierAccessFlags::kCopyDest,
                                             Core::ResourceMemory::kHostWriteThrough });

        void* mapped = stagingBuffer->Map();
        memcpy(mapped, page->m_hostStorage, kTablePageSize);
        stagingBuffer->Unmap();

        graph.AddCopyPass(page->m_deviceStorage.Get(), stagingBuffer.Get());

        StagingPage stagingPage;
        stagingPage.m_buffer = stagingBuffer;
        stagingPage.m_fenceValue = m_fenceValue + 1;
        m_pendingPagesQueue.push_back(stagingPage);
    }


    Core::FenceSyncPoint PageReplicationManager::CloseFrame()
    {
        return Core::FenceSyncPoint{ m_fence, ++m_fenceValue };
    }


    void PageReplicationManager::CheckPendingPages()
    {
        if (m_pendingPagesQueue.empty())
            return;

        const uint64_t completedFenceValue = m_fence->GetCompletedValue();
        while (!m_pendingPagesQueue.empty())
        {
            const StagingPage& page = m_pendingPagesQueue.front();
            if (page.m_fenceValue > completedFenceValue)
                break;

            m_pendingPagesQueue.pop_front();
        }
    }


    TableBase::TableBase(Database* database)
        : m_database(database)
    {
    }


    void TableBase::Update(const uint32_t pageIndex, const uint32_t byteOffset, const festd::span<const std::byte> data)
    {
        FE_Assert(pageIndex < m_pages.size());

        StoragePage* page = m_pages[pageIndex];
        page->Update(byteOffset, data);
        m_database->MarkPageDirty(page);
    }


    Database::Database(Core::ResourcePool* resourcePool)
        : m_resourcePool(resourcePool)
    {
    }


    Database::~Database() = default;


    void Database::MarkPageDirty(const StoragePage* page)
    {
        const uint32_t pageIndex = page->m_globalID;
        FE_Assert(!m_freePages.test(pageIndex));
        FE_Assert(m_pages[pageIndex] == page);
        m_dirtyPages.set(pageIndex);
    }


    StoragePage* Database::AllocatePage()
    {
        auto* page = StoragePage::Allocate(m_resourcePool);
        page->m_globalID = m_pages.size();
        m_pages.push_back(page);
        return page;
    }


    void Database::UploadDirtyPages(Core::FrameGraph& graph, PageReplicationManager& replicationManager)
    {
        Bit::Traverse(m_dirtyPages.view(), [&](const uint32_t pageIndex) {
            replicationManager.UploadPage(graph, m_pages[pageIndex]);
        });
    }
} // namespace FE::Graphics::DB
