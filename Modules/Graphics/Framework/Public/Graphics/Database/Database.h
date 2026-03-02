#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <FeCore/Memory/BuddyAllocator.h>
#include <Graphics/Core/Base.h>
#include <Graphics/Core/RingUploader.h>
#include <Graphics/Database/Base.h>
#include <festd/bit_vector.h>
#include <festd/ring_buffer.h>
#include <festd/vector.h>

namespace FE::Graphics::DB
{
    struct StoragePage final
    {
        void Update(uint32_t byteOffset, const void* data, uint32_t dataSize);

        [[nodiscard]] std::byte* GetHostStorage()
        {
            return m_hostStorage;
        }

        [[nodiscard]] const std::byte* GetHostStorage() const
        {
            return m_hostStorage;
        }

    private:
        friend Database;

        static StoragePage* Allocate(Core::ResourcePool* resourcePool, uint32_t globalID);

        PageReplicationPolicy m_replicationPolicy = PageReplicationPolicy::kDefault;
        uint32_t m_globalID = kInvalidIndex;

        Rc<Core::Buffer> m_deviceStorage;
        std::byte m_hostStorage[kTablePageSize];
    };


    struct TableBase
    {
        FE_RTTI("B0B29217-3D30-4B35-8026-9D0EB91B8FD2");

        TableBase(const TableBase&) = delete;
        TableBase(TableBase&&) = delete;
        TableBase& operator=(const TableBase&) = delete;
        TableBase& operator=(TableBase&&) = delete;

        virtual ~TableBase() = default;

        [[nodiscard]] uint32_t AllocateRowUninitialized();
        void FreeRow(uint32_t rowIndex);

    protected:
        TableBase(Database* database, uint32_t globalID, uint32_t elementCountPerPage);

        void AllocatePage();
        void Update(uint32_t pageIndex, uint32_t byteOffset, const void* data, uint32_t dataSize);

        friend Database;

        Database* m_database = nullptr;
        uint32_t m_globalID = kInvalidIndex;
        Memory::BuddyAllocator::Handle m_devicePageTableAllocation;

        uint32_t m_elementCountPerPage = 0;
        festd::inline_vector<StoragePage*> m_pages;
        festd::bit_vector m_freeRows;
    };


    struct Database final
    {
        Database(Core::ResourcePool* resourcePool);
        ~Database();

        void Update(Core::FrameGraph& graph, const Core::FenceSyncPoint& fence);

        template<class TTable>
        [[nodiscard]] TTable* CreateTable()
        {
            TTable* table = Memory::DefaultNew<TTable>(this, m_tables.size());
            m_tables.push_back(table);
            return table;
        }

    private:
        friend TableBase;

        void MarkPageDirty(const StoragePage* page);

        StoragePage* AllocatePage();
        void FreePage(StoragePage* page);

        void UploadDirtyPages(Core::FrameGraph& graph);
        void UploadPageTables(Core::FrameGraph& graph);

        Core::ResourcePool* m_resourcePool;
        Core::RingUploader m_uploader;

        Memory::BuddyAllocator m_pageTableAllocator;
        Rc<Core::Buffer> m_pageTableDeviceStorage;
        festd::vector<BufferPointer> m_pageTableHostStorage;
        festd::vector<TableBase*> m_tables;

        SegmentedVector<StoragePage*> m_pages;
        festd::bit_vector m_freePages;
        festd::bit_vector m_dirtyPages;
    };
} // namespace FE::Graphics::DB
