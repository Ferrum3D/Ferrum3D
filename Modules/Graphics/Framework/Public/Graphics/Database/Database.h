#pragma once
#include <FeCore/Containers/SegmentedVector.h>
#include <Graphics/Core/Base.h>
#include <Graphics/Database/Base.h>
#include <festd/bit_vector.h>
#include <festd/ring_buffer.h>
#include <festd/vector.h>

namespace FE::Graphics::DB
{
    enum class PageReplicationPolicy
    {
        kCopyAllData = 0, //!< Copy all page data to the device-local memory each frame if modified.

        kDefault = kCopyAllData,
    };


    struct StoragePage final
    {
        void Update(uint32_t byteOffset, festd::span<const std::byte> data);

    private:
        friend Database;
        friend PageReplicationManager;

        static StoragePage* Allocate(Core::ResourcePool* resourcePool);

        PageReplicationPolicy m_replicationPolicy = PageReplicationPolicy::kDefault;
        uint32_t m_globalID = kInvalidIndex;

        Rc<Core::Buffer> m_deviceStorage;
        std::byte* m_hostStorage = nullptr;
    };


    struct PageReplicationManager final
    {
        PageReplicationManager(Core::ResourcePool* resourcePool);

        Core::FenceSyncPoint CloseFrame();

    private:
        friend Database;

        struct StagingPage final
        {
            Rc<Core::Buffer> m_buffer;
            uint64_t m_fenceValue = 0;
        };

        void UploadPage(Core::FrameGraph& graph, const StoragePage* page);

        void CheckPendingPages();

        Core::ResourcePool* m_resourcePool;

        Rc<Core::Fence> m_fence;
        uint64_t m_fenceValue = 0;
        festd::inline_ring_buffer<StagingPage, 32> m_pendingPagesQueue;
    };


    struct TableBase
    {
    protected:
        explicit TableBase(Database* database);

        void Update(uint32_t pageIndex, uint32_t byteOffset, festd::span<const std::byte> data);

        friend Database;

        Database* m_database = nullptr;
        uint32_t m_rowByteSize = 0;
        festd::inline_vector<StoragePage*> m_pages;
        festd::bit_vector m_freeRows;
    };


    struct Database final
    {
        Database(Core::ResourcePool* resourcePool);
        ~Database();

        void MarkPageDirty(const StoragePage* page);
        StoragePage* AllocatePage();

        void UploadDirtyPages(Core::FrameGraph& graph, PageReplicationManager& replicationManager);

    private:
        Core::ResourcePool* m_resourcePool;

        SegmentedVector<StoragePage*> m_pages;
        festd::bit_vector m_freePages;
        festd::bit_vector m_dirtyPages;
    };
} // namespace FE::Graphics::DB
