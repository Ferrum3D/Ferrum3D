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

        Rc<HAL::Image> m_image;
        Rc<HAL::ImageView> m_imageView;
        std::atomic<LoadingState> m_loadingState = LoadingState::kNone;
        std::atomic<uint32_t> m_loadedMipCount = 0;

    protected:
        void Delete() override;

    public:
        explicit ImageAssetStorage(ImageAssetLoader* loader);

        FE_RTTI_Class(ImageAssetStorage, "0C9406E1-44CF-49E1-8B3B-D9E116E10C91");

        static constexpr std::string_view kAssetTypeName = "Image";

        [[nodiscard]] bool IsAnythingLoaded() const override
        {
            return m_loadingState.load(std::memory_order_acquire) >= LoadingState::kHasMips;
        }

        [[nodiscard]] bool IsCompletelyLoaded() const override
        {
            return m_loadingState.load(std::memory_order_acquire) == LoadingState::kCompleted;
        }

        [[nodiscard]] inline HAL::Image* GetImage() const
        {
            std::lock_guard lk{ m_mutex };
            return m_image.Get();
        }

        [[nodiscard]] inline HAL::ImageView* GetImageView() const
        {
            std::lock_guard lk{ m_mutex };
            return m_imageView.Get();
        }
    };
} // namespace FE::Graphics
