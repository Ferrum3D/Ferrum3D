#pragma once
#include <Graphics/Core/Texture.h>
#include <Graphics/Core/Vulkan/Image.h>

namespace FE::Graphics::Vulkan
{
    enum class TextureSubresourceState : uint8_t
    {
        kUndefined,
        kTransferDestination,
        kShaderResource,
        kCount,
    };


    struct Texture final
        : public Core::Texture
        , public Image
    {
        FE_RTTI_Class(Texture, "691EA96F-E1F3-47C5-BF5B-24258DFA57A8");

        ~Texture() override;

        static Texture* Create(Core::Device* device);

        void InitInternal(VmaAllocator allocator, Env::Name name, const Core::ImageDesc& desc);

        const Core::ImageDesc& GetDesc() const override;

        TextureSubresourceState GetSubresourceState(const uint32_t mipIndex, const uint32_t arrayIndex) const
        {
            const uint32_t subresourceIndex = mipIndex + arrayIndex * m_desc.m_mipSliceCount;
            const uint32_t packedIndex = subresourceIndex / PackedSubresourceStates::kCount;

            switch (subresourceIndex % PackedSubresourceStates::kCount)
            {
            default:
                FE_DebugBreak();
                [[fallthrough]];

                // clang-format off
                case 0: return m_subresourceUploadStates[packedIndex].m_state0;
                case 1: return m_subresourceUploadStates[packedIndex].m_state1;
                case 2: return m_subresourceUploadStates[packedIndex].m_state2;
                case 3: return m_subresourceUploadStates[packedIndex].m_state3;
                // clang-format on
            }
        }

        void SetSubresourceState(const uint32_t mipIndex, const uint32_t arrayIndex, const TextureSubresourceState state) const
        {
            FE_AssertDebug(state < TextureSubresourceState::kCount);

            const uint32_t subresourceIndex = mipIndex + arrayIndex * m_desc.m_mipSliceCount;
            const uint32_t packedIndex = subresourceIndex / PackedSubresourceStates::kCount;

            switch (subresourceIndex % PackedSubresourceStates::kCount)
            {
            default:
                FE_DebugBreak();
                [[fallthrough]];

                // clang-format off
                case 0: m_subresourceUploadStates[packedIndex].m_state0 = state; break;
                case 1: m_subresourceUploadStates[packedIndex].m_state1 = state; break;
                case 2: m_subresourceUploadStates[packedIndex].m_state2 = state; break;
                case 3: m_subresourceUploadStates[packedIndex].m_state3 = state; break;
                // clang-format on
            }
        }

    private:
        explicit Texture(Core::Device* device);

        struct PackedSubresourceStates
        {
            TextureSubresourceState m_state0 : 2;
            TextureSubresourceState m_state1 : 2;
            TextureSubresourceState m_state2 : 2;
            TextureSubresourceState m_state3 : 2;

            static constexpr uint32_t kCount = 4;
        };

        mutable festd::inline_vector<PackedSubresourceStates, 8> m_subresourceUploadStates;
    };

    FE_ENABLE_NATIVE_CAST(Texture);
} // namespace FE::Graphics::Vulkan
