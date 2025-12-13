#include <Graphics/Core/Fence.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Database/Database.h>

namespace FE::Graphics::DB
{
    PageReplicationManager::PageReplicationManager(Core::ResourcePool* resourcePool)
        : m_resourcePool(resourcePool)
    {
    }


    void PageReplicationManager::UploadPage(Core::FrameGraph& graph, const StoragePage* page)
    {
        FE_Assert(page->m_replicationPolicy == PageReplicationPolicy::kCopyAllData, "Not implemented");

        CheckPendingPages();

        const Rc stagingPage = m_resourcePool->CreateBuffer("PageReplicationManagerStaging", Core::BufferDesc(kTablePageSize));
    }


    void PageReplicationManager::CheckPendingPages()
    {
        if (m_pendingPagesQueue.empty())
            return;

        while (!m_pendingPagesQueue.empty())
        {
            const StagingPage& page = m_pendingPagesQueue.front();
            if (page.m_fenceValue > m_fence->GetCompletedValue())
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
        return nullptr;
    }
} // namespace FE::Graphics::DB
