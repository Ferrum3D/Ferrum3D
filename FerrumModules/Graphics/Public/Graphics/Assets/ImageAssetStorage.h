#pragma once
#include <FeCore/Assets/AssetStorage.h>
#include <HAL/Image.h>
#include <HAL/ImageView.h>

namespace FE::Graphics
{
    class ImageAssetLoader;

    class ImageAssetStorage : public Assets::AssetStorage
    {
        friend class ImageAssetLoader;

        enum class LoadingState
        {
            kNone,
            kHeaderLoaded,
            kHasMips,
            kCompleted,
        };

        Rc<HAL::Image> m_Image;
        Rc<HAL::ImageView> m_ImageView;
        std::atomic<LoadingState> m_LoadingState = LoadingState::kNone;
        std::atomic<uint32_t> m_LoadedMipCount = 0;

    protected:
        void Delete() override;

    public:
        explicit ImageAssetStorage(ImageAssetLoader* loader);

        FE_RTTI_Class(ImageAssetStorage, "0C9406E1-44CF-49E1-8B3B-D9E116E10C91");

        [[nodiscard]] bool IsAnythingLoaded() const override
        {
            return m_LoadingState.load(std::memory_order_acquire) >= LoadingState::kHasMips;
        }

        [[nodiscard]] bool IsCompletelyLoaded() const override
        {
            return m_LoadingState.load(std::memory_order_acquire) == LoadingState::kCompleted;
        }

        [[nodiscard]] inline HAL::Image* GetImage() const
        {
            std::lock_guard lk{ m_Mutex };
            return m_Image.Get();
        }

        [[nodiscard]] inline HAL::ImageView* GetImageView() const
        {
            std::lock_guard lk{ m_Mutex };
            return m_ImageView.Get();
        }
    };
} // namespace FE::Graphics
