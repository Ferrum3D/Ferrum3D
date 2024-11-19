#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/Containers/ByteBuffer.h>
#include <Graphics/RHI/ShaderModule.h>

namespace FE::Graphics
{
    struct ShaderAssetLoader;

    struct ShaderAssetStorage : public Assets::AssetStorage
    {
        explicit ShaderAssetStorage(ShaderAssetLoader* loader);

        FE_RTTI_Class(ShaderAssetStorage, "AD125813-98AB-4960-A66C-80F5DEF4723C");

        static constexpr std::string_view kAssetTypeName = "Shader";

        [[nodiscard]] bool IsAnythingLoaded() const override
        {
            return m_ready.load(std::memory_order_acquire);
        }

        [[nodiscard]] bool IsCompletelyLoaded() const override
        {
            return m_ready.load(std::memory_order_acquire);
        }

        [[nodiscard]] inline RHI::ShaderModule* GetShaderModule() const
        {
            return m_shaderModule.Get();
        }

    private:
        friend struct ShaderAssetLoader;

        Rc<RHI::ShaderModule> m_shaderModule;
        std::atomic<bool> m_ready = false;

    protected:
        void Delete() override;
    };
} // namespace FE::Graphics
