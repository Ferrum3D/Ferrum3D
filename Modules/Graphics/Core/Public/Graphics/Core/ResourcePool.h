#pragma once
#include <Graphics/Core/Barrier.h>
#include <Graphics/Core/Buffer.h>
#include <Graphics/Core/Texture.h>

namespace FE::Graphics::Core
{
    struct TextureCommitParams final
    {
        BarrierAccessFlags m_bindFlags = BarrierAccessFlags::kNone;
        ResourceMemory m_memory = ResourceMemory::kNotCommitted;
        BarrierLayout m_initialLayout = BarrierLayout::kUndefined;
    };


    struct BufferCommitParams final
    {
        BarrierAccessFlags m_bindFlags = BarrierAccessFlags::kNone;
        ResourceMemory m_memory = ResourceMemory::kNotCommitted;
    };


    struct ResourcePool : public DeviceObject
    {
        ~ResourcePool() override = default;

        FE_RTTI("389492DC-7AE2-4B58-984C-6A1529EDFB41");

        virtual Texture* CreateTexture(Env::Name name, TextureDesc desc) = 0;
        virtual Buffer* CreateBuffer(Env::Name name, BufferDesc desc) = 0;

        Texture* CreateTexture(const Env::Name name, const Format format, const Vector2UInt size, const uint32_t mipCount = 1)
        {
            TextureDesc desc;
            desc.SetSize(size);
            desc.m_dimension = ImageDimension::k2D;
            desc.m_mipSliceCount = mipCount;
            desc.m_imageFormat = format;
            desc.m_sampleCount = 1;
            desc.m_arraySize = 1;
            return CreateTexture(name, desc);
        }

        Texture* CreateTexture(const Env::Name name, const Format format, const Vector3UInt size, const uint32_t mipCount = 1)
        {
            TextureDesc desc;
            desc.SetSize(size);
            desc.m_dimension = ImageDimension::k3D;
            desc.m_mipSliceCount = mipCount;
            desc.m_imageFormat = format;
            desc.m_sampleCount = 1;
            desc.m_arraySize = 1;
            return CreateTexture(name, desc);
        }

        template<class T>
        Buffer* CreateStructuredBuffer(const Env::Name name, const uint32_t elementCount)
        {
            BufferDesc desc;
            desc.m_format = Format::kUndefined;
            desc.m_size = sizeof(T) * elementCount;
            return CreateBuffer(name, desc);
        }

        Buffer* CreateByteAddressBuffer(const Env::Name name, const uint32_t byteSize)
        {
            BufferDesc desc;
            desc.m_format = Format::kUndefined;
            desc.m_size = byteSize;
            return CreateBuffer(name, desc);
        }

        virtual void CommitTextureMemory(Texture* texture, const TextureCommitParams& params) = 0;
        virtual void CommitBufferMemory(Buffer* buffer, const BufferCommitParams& params) = 0;

        virtual void DecommitTextureMemory(Texture* texture) = 0;
        virtual void DecommitBufferMemory(Buffer* buffer) = 0;

        virtual void EndFrame() = 0;
    };
} // namespace FE::Graphics::Core
