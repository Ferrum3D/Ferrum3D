#pragma once
#include <Core/Memory/BuddyAllocator.h>
#include <Graphics/Core/FrameGraph/FrameGraph.h>
#include <Graphics/Core/RingUploader.h>
#include <Shaders/Base/Base.h>
#include <festd/bit_vector.h>
#include <festd/vector.h>

namespace FE::Graphics
{
    struct MaterialInstanceRuntime;

    struct MaterialParameterAllocator final
    {
        static constexpr uint32_t kPageSize = 64 * 1024;

        struct Allocation final
        {
            uint32_t m_pageIndex = kInvalidIndex;
            Memory::BuddyAllocator::Handle m_block;
            BufferPointer m_devicePointer{};

            [[nodiscard]] bool IsValid() const
            {
                return m_pageIndex != kInvalidIndex;
            }
        };

        MaterialParameterAllocator(Core::Device* device, Core::ResourcePool* resourcePool);
        ~MaterialParameterAllocator();

        [[nodiscard]] Allocation Allocate(uint32_t byteSize);
        void Free(Allocation allocation);
        void Write(Allocation allocation, const void* data, uint32_t byteSize);
        void Update(Core::FrameGraph& graph, const Core::FenceSyncPoint& fence);
        void Register(MaterialInstanceRuntime* material);
        void Unregister(MaterialInstanceRuntime* material);

    private:
        struct Page;

        Page* AllocatePage();

        Core::Device* m_device = nullptr;
        Core::ResourcePool* m_resourcePool = nullptr;
        Core::RingUploader m_uploader;
        festd::vector<Page*> m_pages;
        festd::bit_vector m_dirtyPages;
        festd::vector<MaterialInstanceRuntime*> m_materials;
    };
} // namespace FE::Graphics
