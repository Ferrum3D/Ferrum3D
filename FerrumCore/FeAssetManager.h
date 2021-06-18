#pragma once
#include <memory>
#include "CoreUtils.h"
#include <unordered_map>

namespace Ferrum
{
	struct FeRawAssetWeak
	{
		size_t Size{};
		std::weak_ptr<void> Ptr{};

		inline FeRawAssetWeak() = default;

		inline FeRawAssetWeak(size_t size, const std::shared_ptr<void>& data) {
			Size = size;
			Ptr = data;
		}
	};

	struct FeRawAsset
	{
	private:
		friend class FeAssetManager;

		size_t m_Size;
		std::shared_ptr<void> m_Ptr;

		inline FeRawAsset(const FeRawAssetWeak& weakAsset) {
			m_Size = weakAsset.Size;
			m_Ptr = weakAsset.Ptr.lock();
		}

		inline FeRawAsset(size_t size, const std::shared_ptr<void>& data) {
			m_Size = size;
			m_Ptr = data;
		}

		inline FeRawAsset(size_t size, std::shared_ptr<void>&& data) {
			m_Size = size;
			m_Ptr = std::move(data);
		}

	public:
		inline size_t GetSize() {
			return m_Size;
		}

		template<class T>
		inline const T* Read() const {
			return (const T*)Read<void>();
		}

		template<>
		inline const void* Read<void>() const {
			return m_Ptr.get();
		}
	};

	class FE_CORE_API FeAssetManager
	{
		std::unordered_map<std::string, FeRawAssetWeak> m_CachedAssets{}; // TODO: make this weak and thread-safe

		FeRawAsset LoadRawAssetImpl(const std::string& fileName);
	public:
		FeAssetManager();

		FeRawAsset LoadRawAsset(const std::string& fileName, size_t offset, size_t size);

		FeRawAsset LoadRawAsset(const std::string& fileName);
	};
}
