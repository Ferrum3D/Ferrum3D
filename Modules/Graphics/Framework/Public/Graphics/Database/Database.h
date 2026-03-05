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
        void Setup(uint32_t rowCount);
        void Shutdown();

        [[nodiscard]] std::byte* GetHostStorage()
        {
            return m_hostStorage;
        }

        [[nodiscard]] const std::byte* GetHostStorage() const
        {
            return m_hostStorage;
        }

        [[nodiscard]] uint32_t AllocateRows(uint32_t rowCount);
        void FreeRows(uint32_t offset, uint32_t rowCount);

    private:
        friend Database;

        static StoragePage* Allocate(Core::ResourcePool* resourcePool, uint32_t globalID);

        uint32_t m_globalID = kInvalidIndex;

        Rc<Core::Buffer> m_deviceStorage;
        std::byte m_hostStorage[kTablePageSize];

        Memory::BuddyAllocator m_rowAllocator;
    };


    struct TableBase : public Memory::RefCountedObjectBase
    {
        FE_RTTI("B0B29217-3D30-4B35-8026-9D0EB91B8FD2");

        struct RowRangeHandle final
        {
            uint32_t m_offset = kInvalidIndex;
            uint32_t m_size = 0;
        };

        TableBase(const TableBase&) = delete;
        TableBase(TableBase&&) = delete;
        TableBase& operator=(const TableBase&) = delete;
        TableBase& operator=(TableBase&&) = delete;

        ~TableBase() override;

        [[nodiscard]] uint32_t AllocateRowUninitialized();
        void Free(uint32_t rowIndex);

        [[nodiscard]] RowRangeHandle AllocateRowsUninitialized(uint32_t rowCount);
        void Free(RowRangeHandle rowRange);

    protected:
        TableBase(Database* database, uint32_t rowsPerPage);

        void AllocatePage();

        friend Database;

        Database* m_database = nullptr;
        uint32_t m_id = kInvalidIndex;
        Memory::BuddyAllocator::Handle m_devicePageTableAllocation;

        uint32_t m_rowsPerPage = 0;
        festd::inline_vector<StoragePage*> m_pages;
    };


    struct Database final
    {
        Database(Core::ResourcePool* resourcePool);
        ~Database();

        void Update(Core::FrameGraph& graph, const Core::FenceSyncPoint& fence);

        void MarkPageDirty(const StoragePage* page);

    private:
        friend TableBase;

        uint32_t RegisterTable(TableBase* table);
        void UnregisterTable(const TableBase* table);

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
        festd::bit_vector m_freeTables;

        SegmentedVector<StoragePage*> m_pages;
        festd::bit_vector m_freePages;
        festd::bit_vector m_dirtyPages;
    };
} // namespace FE::Graphics::DB
