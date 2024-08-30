#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <FeCore/Containers/ByteBuffer.h>
#include <HAL/ShaderModule.h>

namespace FE::Graphics
{
    class ShaderAssetLoader;

    class ShaderAssetStorage : public Assets::AssetStorage
    {
        friend class ShaderAssetLoader;

        Rc<HAL::ShaderModule> m_ShaderModule;
        std::atomic<bool> m_Ready = false;

    protected:
        void Delete() override;

    public:
        explicit ShaderAssetStorage(ShaderAssetLoader* loader);

        FE_RTTI_Class(ShaderAssetStorage, "AD125813-98AB-4960-A66C-80F5DEF4723C");

        static constexpr std::string_view kAssetTypeName = "Shader";

        [[nodiscard]] bool IsAnythingLoaded() const override
        {
            return m_Ready.load(std::memory_order_acquire);
        }

        [[nodiscard]] bool IsCompletelyLoaded() const override
        {
            return m_Ready.load(std::memory_order_acquire);
        }

        [[nodiscard]] inline HAL::ShaderModule* GetShaderModule() const
        {
            return m_ShaderModule.Get();
        }
    };
} // namespace FE::Graphics
