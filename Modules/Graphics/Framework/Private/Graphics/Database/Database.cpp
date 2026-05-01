#include <FeCore/Memory/PoolAllocator.h>
#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Database/Database.h>

namespace FE::Graphics::DB
{
    namespace
    {
        constexpr uint32_t kPageTableStorageByteSize = 2 * 1024 * 1024;
        constexpr uint32_t kPageTableSize = kPageTableStorageByteSize / sizeof(BufferPointer);

        Memory::SpinLockedPool<StoragePage> GStoragePagePool{ "Graphics/DB/StoragePagePool", 2 * 1024 * 1024 };
    } // namespace


    void StoragePage::Setup(const uint32_t rowCount)
    {
        m_rowAllocator.Setup(rowCount);
    }


    void StoragePage::Shutdown()
    {
        m_rowAllocator.Shutdown();
    }


    uint32_t StoragePage::AllocateRows(const uint32_t rowCount)
    {
        return m_rowAllocator.Allocate(rowCount, 1).m_offset;
    }


    void StoragePage::FreeRows(const uint32_t offset, const uint32_t rowCount)
    {
        const uint32_t blockSize = m_rowAllocator.QuantizeAllocationSize(rowCount);

        Memory::BuddyAllocator::Handle allocation;
        allocation.m_offset = offset;
        allocation.m_level = m_rowAllocator.LevelForBlockSize(blockSize);
        m_rowAllocator.Free(allocation);
    }


    StoragePage* StoragePage::Allocate(Core::ResourcePool* resourcePool, const uint32_t globalID)
    {
        StoragePage* page = GStoragePagePool.New();
        page->m_deviceStorage = resourcePool->CreateByteAddressBuffer("StoragePage", kTablePageSize);
        page->m_globalID = globalID;

        Core::ResourceCommitParams commitParams;
        commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
        commitParams.m_bindFlags = Core::BarrierAccessFlags::kCopySource | Core::BarrierAccessFlags::kCopyDest
            | Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite;
        resourcePool->CommitBufferMemory(page->m_deviceStorage.Get(), commitParams);

        return page;
    }


    uint32_t TableBase::AllocateRowUninitialized()
    {
        const auto range = AllocateRowsUninitialized(1);
        FE_AssertDebug(range.m_size == 1);
        return range.m_offset;
    }


    void TableBase::Free(const uint32_t rowIndex)
    {
        Free({ rowIndex, 1 });
    }


    TableBase::RowRangeHandle TableBase::AllocateRowsUninitialized(const uint32_t rowCount)
    {
        FE_Assert(rowCount <= m_rowsPerPage);
        for (uint32_t pageIndex = 0; pageIndex < m_pages.size(); ++pageIndex)
        {
            StoragePage* page = m_pages[pageIndex];
            const uint32_t offset = page->AllocateRows(rowCount);
            if (offset != kInvalidIndex)
                return { pageIndex * m_rowsPerPage + offset, rowCount };
        }

        AllocatePage();
        const uint32_t offset = m_pages.back()->AllocateRows(rowCount);
        FE_Assert(offset != kInvalidIndex);
        return { (m_pages.size() - 1) * m_rowsPerPage + offset, rowCount };
    }


    void TableBase::Free(const RowRangeHandle rowRange)
    {
        const uint32_t pageIndex = rowRange.m_offset / m_rowsPerPage;
        const uint32_t offset = rowRange.m_offset % m_rowsPerPage;
        m_pages[pageIndex]->FreeRows(offset, rowRange.m_size);
    }


    uint32_t TableBase::GetReservedRowCount() const
    {
        return m_rowsPerPage * m_pages.size();
    }


    TableBase::TableBase(Database* database, const uint32_t rowsPerPage)
        : m_database(database)
        , m_rowsPerPage(rowsPerPage)
    {
        m_id = m_database->RegisterTable(this);
    }


    TableBase::~TableBase()
    {
        for (StoragePage* page : m_pages)
            m_database->FreePage(page);

        m_database->UnregisterTable(this);
        m_id = kInvalidIndex;
    }


    void TableBase::AllocatePage()
    {
        const uint32_t pageCount = m_pages.size() + 1;
        if (m_devicePageTableAllocation
            && m_database->m_pageTableAllocator.GetUsableSize(m_devicePageTableAllocation) < pageCount)
        {
            m_database->m_pageTableAllocator.Free(m_devicePageTableAllocation);
            m_devicePageTableAllocation.Invalidate();
        }

        if (!m_devicePageTableAllocation)
        {
            m_devicePageTableAllocation = m_database->m_pageTableAllocator.Allocate(pageCount);
            FE_Assert(m_devicePageTableAllocation.IsValid());
        }

        StoragePage* page = m_database->AllocatePage();
        page->Setup(m_rowsPerPage);
        m_pages.push_back(page);
        FE_Assert(m_pages.size() == pageCount);
    }


    Database::Database(Core::ResourcePool* resourcePool)
        : m_resourcePool(resourcePool)
    {
        m_uploader.Setup("GPUDatabaseUploader", resourcePool, 4 * 1024 * 1024);
        m_pageTableDeviceStorage = resourcePool->CreateByteAddressBuffer("GPUDatabasePageTable", kPageTableStorageByteSize);

        Core::ResourceCommitParams commitParams;
        commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
        commitParams.m_bindFlags = Core::BarrierAccessFlags::kCopySourceAndDest | Core::BarrierAccessFlags::kShaderRead;
        resourcePool->CommitBufferMemory(m_pageTableDeviceStorage.Get(), commitParams);

        m_pageTableHostStorage.resize(kPageTableSize);
        m_pageTableAllocator.Setup(kPageTableSize);
    }


    Database::~Database()
    {
        uint32_t freePageCount = 0;
        Bit::Traverse(m_freePages.view(), [&](const uint32_t pageIndex) {
            GStoragePagePool.Delete(m_pages[pageIndex]);
            ++freePageCount;
        });

        FE_Assert(freePageCount == m_pages.size());

        m_pages.clear();
        m_resourcePool->DecommitBufferMemory(m_pageTableDeviceStorage.Get());
    }


    void Database::MarkPageDirty(const StoragePage* page)
    {
        const uint32_t pageIndex = page->m_globalID;
        FE_AssertDebug(!m_freePages.test(pageIndex));
        FE_AssertDebug(m_pages[pageIndex] == page);
        m_dirtyPages.set(pageIndex);
    }


    uint32_t Database::RegisterTable(TableBase* table)
    {
        uint32_t tableIndex = m_freeTables.find_first();
        if (tableIndex != kInvalidIndex)
        {
            m_freeTables.reset(tableIndex);
            m_tables[tableIndex] = table;
            return tableIndex;
        }

        tableIndex = m_tables.size();
        m_tables.push_back(table);

        if (m_freeTables.size() < m_tables.size())
            m_freeTables.resize(m_freeTables.size() + 32, false);

        return tableIndex;
    }


    void Database::UnregisterTable(const TableBase* table)
    {
        FE_Assert(m_tables[table->m_id] == table);
        FE_Assert(!m_freeTables.test(table->m_id));
        m_freeTables.set(table->m_id);
        m_tables[table->m_id] = nullptr;
    }


    StoragePage* Database::AllocatePage()
    {
        const uint32_t freePageIndex = m_freePages.find_first();
        if (freePageIndex != kInvalidIndex)
        {
            m_freePages.reset(freePageIndex);
            return m_pages[freePageIndex];
        }

        StoragePage* page = StoragePage::Allocate(m_resourcePool, m_pages.size());
        m_pages.push_back(page);

        if (m_dirtyPages.size() < m_pages.size())
        {
            m_dirtyPages.resize(m_dirtyPages.size() + 512, false);
            m_freePages.resize(m_freePages.size() + 512, false);
        }

        return page;
    }


    void Database::FreePage(StoragePage* page)
    {
        FE_Assert(!m_freePages.test(page->m_globalID));
        FE_Assert(m_pages[page->m_globalID] == page);
        m_freePages.set(page->m_globalID);
        page->Shutdown();
    }


    void Database::Update(Core::FrameGraph& graph, const Core::FenceSyncPoint& fence)
    {
        FE_FG_SCOPE(graph, "Database::Update");

        UploadDirtyPages(graph);
        UploadPageTables(graph);
        if (fence.m_fence != nullptr)
            m_uploader.CloseFrame(fence);
    }


    void Database::UploadDirtyPages(Core::FrameGraph& graph)
    {
        FE_FG_SCOPE(graph, "UploadDirtyPages");

        Bit::Traverse(m_dirtyPages.view(), [&](const uint32_t pageIndex) {
            const StoragePage* page = m_pages[pageIndex];
            FE_Verify(m_uploader.Upload(graph, page->m_deviceStorage.Get(), page->m_hostStorage, kTablePageSize));
        });

        // Shader accesses to the Database pages are not known to the FrameGraph.
        // So we have to manually transition them to a readable state in the beginning of the frame.
        Bit::Traverse(m_dirtyPages.view(), [&](const uint32_t pageIndex) {
            const StoragePage* page = m_pages[pageIndex];
            auto* barrierPassDesc = graph.AllocatePassData<Core::BufferAccessPassDesc>();
            barrierPassDesc->m_access = { page->m_deviceStorage.Get(),
                                          Core::BarrierSyncFlags::kAllShading,
                                          Core::BarrierAccessFlags::kShaderRead };
            graph.AddPass("BarrierPass", barrierPassDesc);
        });

        m_dirtyPages.reset();
    }


    void Database::UploadPageTables(Core::FrameGraph& graph)
    {
        FE_FG_SCOPE(graph, "UploadPageTables");

        if (m_tables.empty())
            return;

        constexpr auto kUploadOptions = Core::RingUploader::Options::kDisableBarriers;

        Core::DescriptorManager* descriptorManager = graph.GetDescriptorManager();
        const uint32_t pageTableDescriptorIndex = descriptorManager->ReserveDescriptor(m_pageTableDeviceStorage.Get());
        descriptorManager->CommitResourceDescriptor(pageTableDescriptorIndex, Core::DescriptorType::kSRV);
        const uint64_t pageTableAddress = descriptorManager->GetDeviceAddress(pageTableDescriptorIndex);

        // We place barriers manually here as we know that the page tables do not overlap in memory.
        // Currently, our FrameGraph cannot figure it out on its own.

        {
            auto* passData = graph.AllocatePassData<Core::BufferAccessPassDesc>();
            passData->m_access = { m_pageTableDeviceStorage.Get(),
                                   Core::BarrierSyncFlags::kCopy,
                                   Core::BarrierAccessFlags::kCopyDest };
            graph.AddPass("PrologueBarrier", passData);
        }

        for (const TableBase* table : m_tables)
        {
            const uint32_t pageCount = table->m_pages.size();
            const uint32_t offset = table->m_devicePageTableAllocation.m_offset;
            const uint32_t byteOffset = offset * sizeof(BufferPointer);
            for (uint32_t i = 0; i < pageCount; ++i)
            {
                const uint64_t address = pageTableAddress + byteOffset;
                m_pageTableHostStorage[offset + i] = BufferPointer{ address };
            }

            const uint32_t pageTableByteSize = pageCount * sizeof(BufferPointer);
            const Core::BufferSlice destinationRange{ byteOffset, pageTableByteSize };
            const Core::BufferView destination{ m_pageTableDeviceStorage.Get(), destinationRange };
            const BufferPointer* source = m_pageTableHostStorage.data() + offset;
            FE_Verify(m_uploader.Upload(graph, destination, source, pageTableByteSize, kUploadOptions));
        }

        {
            auto* passData = graph.AllocatePassData<Core::BufferAccessPassDesc>();
            passData->m_access = { m_pageTableDeviceStorage.Get(),
                                   Core::BarrierSyncFlags::kAllShading,
                                   Core::BarrierAccessFlags::kShaderRead };
            graph.AddPass("EpilogueBarrier", passData);
        }
    }
} // namespace FE::Graphics::DB
