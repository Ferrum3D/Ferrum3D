#pragma once
#include <Graphics/Core/Barrier.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::Core
{
    struct ResourceCommitParams final
    {
        BarrierAccessFlags m_bindFlags = BarrierAccessFlags::kNone;
        ResourceMemory m_memory = ResourceMemory::kNotCommitted;
    };


    struct ResourcePool : public DeviceObject
    {
        ~ResourcePool() override = default;

        FE_RTTI("389492DC-7AE2-4B58-984C-6A1529EDFB41");

        virtual void CommitTextureMemory(Texture* texture, const ResourceCommitParams& params) = 0;
        virtual void CommitBufferMemory(Buffer* buffer, const ResourceCommitParams& params) = 0;

        virtual void DecommitTextureMemory(Texture* texture) = 0;
        virtual void DecommitBufferMemory(Buffer* buffer) = 0;

        virtual void EndFrame() = 0;
    };
} // namespace FE::Graphics::Core
