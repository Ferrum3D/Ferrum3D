#pragma once
#include <Utils/CoreUtils.h>
#include <memory>
#include <unordered_map>

namespace FE
{
    struct RawAssetWeak
    {
        size_t Size{};
        std::weak_ptr<void> Ptr{};

        inline RawAssetWeak() = default;

        inline RawAssetWeak(size_t size, const std::shared_ptr<void>& data)
        {
            Size = size;
            Ptr  = data;
        }
    };

    struct RawAsset
    {
    private:
        friend class AssetManager;

        size_t m_Size;
        std::shared_ptr<void> m_Ptr;

        inline RawAsset(const RawAssetWeak& weakAsset)
        {
            m_Size = weakAsset.Size;
            m_Ptr  = weakAsset.Ptr.lock();
        }

        inline RawAsset(size_t size, const std::shared_ptr<void>& data)
        {
            m_Size = size;
            m_Ptr  = data;
        }

        inline RawAsset(size_t size, std::shared_ptr<void>&& data)
        {
            m_Size = size;
            m_Ptr  = std::move(data);
        }

    public:
        inline size_t GetSize()
        {
            return m_Size;
        }

        template<class T>
        inline const T* Read() const
        {
            return (const T*)Read<void>();
        }

        template<>
        inline const void* Read<void>() const
        {
            return m_Ptr.get();
        }
    };

    class AssetManager
    {
        std::unordered_map<std::string, RawAssetWeak> m_CachedAssets{}; // TODO: make this weak and thread-safe

        RawAsset LoadRawAssetImpl(const std::string& fileName);

    public:
        AssetManager();

        RawAsset LoadRawAsset(const std::string& fileName, size_t offset, size_t size);

        RawAsset LoadRawAsset(const std::string& fileName);
    };
} // namespace FE
