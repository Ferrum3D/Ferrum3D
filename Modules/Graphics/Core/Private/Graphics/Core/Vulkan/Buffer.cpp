#include <Graphics/Core/Vulkan/Base/BaseTypes.h>
#include <Graphics/Core/Vulkan/Base/Config.h>
#include <Graphics/Core/Vulkan/Buffer.h>
#include <Graphics/Core/Vulkan/Device.h>
#include <Graphics/Core/Vulkan/Format.h>
#include <Graphics/Core/Vulkan/ResourcePool.h>

namespace FE::Graphics::Vulkan
{
    namespace
    {
        VkBufferUsageFlags GetBufferUsage(const Core::BarrierAccessFlags accessFlags, const bool isTexelBuffer)
        {
            FE_Assert((accessFlags & Core::BarrierAccessFlags::kAllBufferAccessMask) == accessFlags);

            VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

            if (Bit::AnySet(accessFlags, Core::BarrierAccessFlags::kShaderRead | Core::BarrierAccessFlags::kShaderWrite))
                usage |= isTexelBuffer ? VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kIndexBuffer))
                usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kVertexBuffer))
                usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kConstantBuffer))
                usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kIndirectArgument))
                usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kAccelerationStructureRead))
                usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
            if (Bit::AllSet(accessFlags, Core::BarrierAccessFlags::kAccelerationStructureWrite))
                usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;

            return usage;
        }
    } // namespace


    FE_DECLARE_VULKAN_OBJECT_POOL(Buffer);


    Buffer* Buffer::Create(Core::Device* device, const Env::Name name, const Core::BufferDesc desc)
    {
        FE_PROFILER_ZONE();

        return Rc<Buffer>::Allocate(&GBufferPool, [device, name, desc](void* memory) {
            return new (memory) Buffer(device, name, desc);
        });
    }


    Buffer::Buffer(Core::Device* device, const Env::Name name, const Core::BufferDesc desc)
    {
        m_device = device;
        m_name = name;
        m_desc = desc;
        m_type = Core::ResourceType::kBuffer;
        Register();
    }


    void Buffer::UpdateDebugNames() const
    {
        if (m_instance == nullptr)
            return;

        auto* bufferInstance = Rtti::AssertCast<BufferInstance*>(m_instance);

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
        nameInfo.objectHandle = reinterpret_cast<uint64_t>(bufferInstance->m_buffer);
        nameInfo.pObjectName = m_name.c_str();
        VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));

        if (bufferInstance->m_view)
        {
            nameInfo.objectType = VK_OBJECT_TYPE_BUFFER_VIEW;
            nameInfo.objectHandle = reinterpret_cast<uint64_t>(bufferInstance->m_view);
            nameInfo.pObjectName = m_name.c_str();
            VerifyVk(vkSetDebugUtilsObjectNameEXT(NativeCast(m_device), &nameInfo));
        }

        if (bufferInstance->m_vmaAllocation)
        {
            vmaSetAllocationName(ImplCast(bufferInstance->m_pool)->GetAllocator(),
                                 bufferInstance->m_vmaAllocation,
                                 m_name.c_str());
        }
    }


    void* Buffer::Map()
    {
        FE_PROFILER_ZONE();

        FE_Assert(m_instance);

        const auto* bufferInstance = Rtti::AssertCast<BufferInstance*>(m_instance);
        FE_Assert(bufferInstance->m_memoryStatus == Core::ResourceMemory::kHostRandomAccess
                  || bufferInstance->m_memoryStatus == Core::ResourceMemory::kHostWriteThrough);

        const VmaAllocator allocator = ImplCast(bufferInstance->m_pool)->GetAllocator();
        const VmaAllocation allocation = bufferInstance->m_vmaAllocation;

        void* result;
        VerifyVk(vmaMapMemory(allocator, allocation, &result));
        return result;
    }


    void Buffer::Unmap()
    {
        const auto* bufferInstance = Rtti::AssertCast<BufferInstance*>(m_instance);
        const VmaAllocator allocator = ImplCast(bufferInstance->m_pool)->GetAllocator();
        const VmaAllocation allocation = bufferInstance->m_vmaAllocation;
        vmaUnmapMemory(allocator, allocation);
    }


    void Buffer::DecommitMemory()
    {
        if (m_instance == nullptr)
            return;

        FE_Assert(m_instance->m_pool, "Externally created buffers not implemented");
        m_instance->m_pool->DecommitBufferMemory(this);
    }


    void Buffer::CommitInternal(ResourcePool* resourcePool, const Core::BufferCommitParams params)
    {
        FE_PROFILER_ZONE();

        FE_Assert(m_instance == nullptr);

        m_instance = BufferInstance::Create();
        m_instance->m_pool = resourcePool;
        m_instance->m_bindFlags = params.m_bindFlags;
        m_instance->m_type = m_type;

        Common::SubresourceState& initialState = m_instance->m_subresourceStates.emplace_back();
        initialState.m_value = 0;

        const bool isTexelBuffer = m_desc.m_format != Core::Format::kUndefined;

        VkBufferCreateInfo bufferCI{};
        bufferCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferCI.size = m_desc.m_size;
        bufferCI.usage = GetBufferUsage(params.m_bindFlags, isTexelBuffer);

        VmaAllocationCreateInfo allocationCI{};
        allocationCI.usage = VMA_MEMORY_USAGE_AUTO;

        switch (params.m_memory)
        {
        default:
        case Core::ResourceMemory::kNotCommitted:
            FE_DebugBreak();
            [[fallthrough]];

        case Core::ResourceMemory::kDeviceLocal:
            allocationCI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            break;

        case Core::ResourceMemory::kHostRandomAccess:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            break;

        case Core::ResourceMemory::kHostWriteThrough:
            allocationCI.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
            break;
        }

        m_instance->m_memoryStatus = params.m_memory;

        const VmaAllocator allocator = resourcePool->GetAllocator();
        auto* bufferInstance = Rtti::AssertCast<BufferInstance*>(m_instance);
        VerifyVk(vmaCreateBuffer(allocator,
                                 &bufferCI,
                                 &allocationCI,
                                 &bufferInstance->m_buffer,
                                 &bufferInstance->m_vmaAllocation,
                                 nullptr));

        if (isTexelBuffer)
        {
            VkBufferViewCreateInfo viewCI{};
            viewCI.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            viewCI.buffer = bufferInstance->m_buffer;
            viewCI.format = Translate(m_desc.m_format);
            viewCI.offset = 0;
            viewCI.range = m_desc.m_size;
            VerifyVk(vkCreateBufferView(NativeCast(m_device), &viewCI, nullptr, &bufferInstance->m_view));
        }

        UpdateDebugNames();
    }


    void Buffer::SwapInternal(BufferInstance*& instance)
    {
        if (instance != nullptr)
        {
            FE_Assert(m_desc == instance->m_bufferDesc);
            FE_Assert(instance->m_memoryStatus != Core::ResourceMemory::kNotCommitted);
        }

        festd::swap(m_instance, instance);
    }


    Buffer::~Buffer()
    {
        DecommitMemory();
    }
} // namespace FE::Graphics::Vulkan
