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


    void StoragePage::Update(const uint32_t byteOffset, const void* data, const uint32_t dataSize)
    {
        memcpy(m_hostStorage + byteOffset, data, dataSize);
    }


    StoragePage* StoragePage::Allocate(Core::ResourcePool* resourcePool, const uint32_t globalID)
    {
        StoragePage* page = GStoragePagePool.New();
        page->m_deviceStorage = resourcePool->CreateByteAddressBuffer("StoragePage", kTablePageSize);
        page->m_globalID = globalID;

        Core::BufferCommitParams commitParams;
        commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
        commitParams.m_bindFlags = Core::BarrierAccessFlags::kCopySource | Core::BarrierAccessFlags::kCopyDest
            | Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite;
        resourcePool->CommitBufferMemory(page->m_deviceStorage.Get(), commitParams);

        return page;
    }


    TableBase::TableBase(Database* database, const uint32_t globalID)
        : m_database(database)
        , m_globalID(globalID)
    {
    }


    void TableBase::AllocatePage()
    {
        const uint32_t pageCount = m_pages.size() + 1;
        if (m_devicePageTableAllocation && m_devicePageTableAllocation.GetSize() < pageCount)
        {
            m_database->m_pageTableAllocator.Free(m_devicePageTableAllocation);
            m_devicePageTableAllocation.Invalidate();
        }

        if (!m_devicePageTableAllocation)
        {
            const uint32_t reservedPageCount = Math::Max(2u, pageCount);
            const uint32_t allocationSize = Math::CeilPowerOfTwo(reservedPageCount);
            m_devicePageTableAllocation = m_database->m_pageTableAllocator.Allocate(allocationSize, 1);
        }

        StoragePage* page = m_database->AllocatePage();
        m_pages.push_back(page);
        FE_Assert(m_pages.size() == pageCount);
    }


    void TableBase::Update(const uint32_t pageIndex, const uint32_t byteOffset, const void* data, const uint32_t dataSize)
    {
        FE_Assert(pageIndex < m_pages.size());

        StoragePage* page = m_pages[pageIndex];
        page->Update(byteOffset, data, dataSize);
        m_database->MarkPageDirty(page);
    }


    Database::Database(Core::ResourcePool* resourcePool)
        : m_resourcePool(resourcePool)
    {
        m_uploader.Setup("GPUDatabaseUploader", resourcePool, 4 * 1024 * 1024);
        m_pageTableDeviceStorage = resourcePool->CreateByteAddressBuffer("GPUDatabasePageTable", kPageTableStorageByteSize);

        Core::BufferCommitParams commitParams;
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
        FE_Assert(!m_freePages.test(pageIndex));
        FE_Assert(m_pages[pageIndex] == page);
        m_dirtyPages.set(pageIndex);
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
    }


    void Database::Update(Core::FrameGraph& graph, const Core::FenceSyncPoint& fence)
    {
        FE_FG_SCOPE(graph, "Database::Update");

        UploadDirtyPages(graph);
        UploadPageTables(graph);
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
