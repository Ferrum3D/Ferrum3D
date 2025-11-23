#include <Graphics/Core/Fence.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Database/Database.h>

namespace FE::Graphics::DB
{
    PageReplicationManager::PageReplicationManager(Core::ResourcePool* resourcePool)
        : m_resourcePool(resourcePool)
    {
    }


    void PageReplicationManager::UploadPage(Core::FrameGraphBuilder& builder, const StoragePage* page)
    {
        FE_Assert(page->m_replicationPolicy == PageReplicationPolicy::kCopyAllData, "Not implemented");

        Rc<Core::Buffer> stagingPage;
        if (m_freeStagingPages.empty())
            CheckPendingPages();

        if (!m_freeStagingPages.empty())
        {
            stagingPage = m_freeStagingPages.back();
            m_freeStagingPages.pop_back();
        }

        auto accessType = Core::BufferAccessType::kTransferSource;
        if (stagingPage == nullptr)
        {
            const uint32_t bufferIndex = m_bufferIndex++;
            const Env::Name bufferName = Fmt::FormatName("PageReplicationManager_{}", bufferIndex);
            const Core::BufferDesc desc{ kTablePageSize, Core::BindFlags::kNone, Core::ResourceMemory::kHostWriteThrough };
            stagingPage = m_resourcePool->CreateBuffer(bufferName, desc);
            accessType = Core::BufferAccessType::kUndefined;
        }

        const auto pass = builder.AddPass("UploadDatabasePage");
        const auto stagingPageHandle = builder.GetGraph().ImportBuffer(stagingPage.Get(), accessType);
        const auto destinationPageHandle =
            builder.GetGraph().ImportBuffer(page->m_deviceStorage.Get(), Core::BufferAccessType::kUndefined);
        const auto source = pass.Read(stagingPageHandle, Core::BufferReadType::kTransferSource);
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

            m_freeStagingPages.push_back(page.m_buffer);
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
