#include <Graphics/Core/DescriptorManager.h>
#include <Graphics/Core/Device.h>
#include <Graphics/Core/ResourcePool.h>
#include <Graphics/Materials/MaterialInstance.h>
#include <Graphics/Materials/ParameterAllocator.h>

namespace FE::Graphics
{
    struct MaterialParameterAllocator::Page final
    {
        Rc<Core::Buffer> m_buffer;
        Memory::BuddyAllocator m_allocator;
        uint64_t m_deviceAddress = 0;
        alignas(16) std::byte m_hostData[kPageSize]{};
    };


    MaterialParameterAllocator::MaterialParameterAllocator(Core::Device* device, Core::ResourcePool* resourcePool,
                                                           Core::DescriptorManager* descriptorManager)
        : m_device(device)
        , m_resourcePool(resourcePool)
        , m_descriptorManager(descriptorManager)
    {
        m_uploader.Setup("MaterialParameterUploader", resourcePool, 4 * 1024 * 1024);
    }


    MaterialParameterAllocator::~MaterialParameterAllocator()
    {
        for (Page* page : m_pages)
        {
            page->m_allocator.Shutdown();
            m_resourcePool->DecommitBufferMemory(page->m_buffer.Get());
            Memory::DefaultDelete(page);
        }
    }


    MaterialParameterAllocator::Page* MaterialParameterAllocator::AllocatePage()
    {
        Page* page = Memory::DefaultNew<Page>();
        const uint32_t pageIndex = m_pages.size();
        page->m_allocator.Setup(kPageSize, 16);
        page->m_buffer =
            Core::Buffer::CreateByteAddress(m_device, Fmt::FormatName("MaterialParameterPage_{}", pageIndex), kPageSize);

        Core::ResourceCommitParams commitParams;
        commitParams.m_memory = Core::ResourceMemory::kDeviceLocal;
        commitParams.m_bindFlags = Core::BarrierAccessFlags::kCopyDest | Core::BarrierAccessFlags::kShaderRead;
        m_resourcePool->CommitBufferMemory(page->m_buffer.Get(), commitParams);

        const uint32_t descriptorIndex = m_descriptorManager->ReserveDescriptor(page->m_buffer.Get());
        m_descriptorManager->CommitResourceDescriptor(descriptorIndex, Core::DescriptorType::kSRV);
        page->m_deviceAddress = m_descriptorManager->GetDeviceAddress(descriptorIndex);
        m_pages.push_back(page);
        if (m_dirtyPages.size() < m_pages.size())
            m_dirtyPages.resize(m_pages.size(), false);
        return page;
    }


    MaterialParameterAllocator::Allocation MaterialParameterAllocator::Allocate(const uint32_t byteSize)
    {
        FE_Assert(byteSize > 0 && byteSize <= kPageSize);
        for (uint32_t pageIndex = 0; pageIndex < m_pages.size(); ++pageIndex)
        {
            const Memory::BuddyAllocator::Handle block = m_pages[pageIndex]->m_allocator.Allocate(byteSize, 16);
            if (block.IsValid())
                return { pageIndex, block, BufferPointer{ m_pages[pageIndex]->m_deviceAddress + block.m_offset } };
        }

        Page* page = AllocatePage();
        const Memory::BuddyAllocator::Handle block = page->m_allocator.Allocate(byteSize, 16);
        FE_Assert(block.IsValid());
        return { static_cast<uint32_t>(m_pages.size() - 1), block, BufferPointer{ page->m_deviceAddress + block.m_offset } };
    }


    void MaterialParameterAllocator::Free(const Allocation allocation)
    {
        FE_Assert(allocation.IsValid() && allocation.m_pageIndex < m_pages.size());
        m_pages[allocation.m_pageIndex]->m_allocator.Free(allocation.m_block);
    }


    void MaterialParameterAllocator::Write(const Allocation allocation, const void* data, const uint32_t byteSize)
    {
        FE_Assert(allocation.IsValid() && allocation.m_pageIndex < m_pages.size());
        Page* page = m_pages[allocation.m_pageIndex];
        FE_Assert(byteSize <= page->m_allocator.GetUsableSize(allocation.m_block));
        memcpy(page->m_hostData + allocation.m_block.m_offset, data, byteSize);
        m_dirtyPages.set(allocation.m_pageIndex);
    }


    void MaterialParameterAllocator::Update(Core::FrameGraph& graph, const Core::FenceSyncPoint& fence)
    {
        for (MaterialInstanceRuntime* material : m_materials)
            material->RefreshDescriptors();

        Bit::Traverse(m_dirtyPages.view(), [&](const uint32_t pageIndex) {
            Page* page = m_pages[pageIndex];
            FE_Verify(m_uploader.Upload(graph, page->m_buffer.Get(), page->m_hostData, kPageSize));

            auto* barrier = graph.AllocatePassData<Core::BufferAccessPassDesc>();
            barrier->m_access = { page->m_buffer.Get(),
                                  Core::BarrierSyncFlags::kAllShading,
                                  Core::BarrierAccessFlags::kShaderRead };
            graph.AddPass("MaterialParameterBarrier", barrier);
        });
        m_dirtyPages.reset();
        m_uploader.CloseFrame(fence);
    }


    void MaterialParameterAllocator::Register(MaterialInstanceRuntime* material)
    {
        m_materials.push_back(material);
    }


    void MaterialParameterAllocator::Unregister(MaterialInstanceRuntime* material)
    {
        const auto iter = festd::find(m_materials, material);
        FE_Assert(iter != m_materials.end());
        m_materials.erase_unsorted(iter);
    }
} // namespace FE::Graphics
