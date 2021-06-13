#pragma once
#include "CoreUtils.h"
#include "FeLog.h"

namespace Ferrum
{
	inline constexpr uint32_t FeStackAllocatorAlign = 16;

	class FeStackAllocator
	{
		uint8_t* m_Memory{};
		size_t m_MemorySize{};
		size_t m_Allocated{};

	public:
		inline FeStackAllocator(size_t size) {
			Resize(size);
		}

		inline void Resize(size_t size) {
			FE_ASSERT_MSG(m_Allocated == 0, "Couldn't resize a non-empty allocator");
			size = FeMakeAlignment<FeStackAllocatorAlign>(size);
			m_MemorySize = size;
			if (m_Memory)
				delete m_Memory;
			m_Memory = new uint8_t[size];
		}

		inline void Reset() {
			m_Allocated = 0;
		}

		inline void* Malloc(size_t size) {
			uint8_t* ptr = m_Memory + m_Allocated;
			m_Allocated += size;
			FE_ASSERT_MSG(m_Allocated <= m_MemorySize, "Allocator overflow");
			return ptr;
		}

		template<class T, class ...Args>
		inline T* Allocate(Args&&... args) {
			return new (Malloc(sizeof(T))) T(args...);
		}

		template<class T>
		inline T* AllocateArray(size_t length) {
			return static_cast<T*>(Malloc(length * sizeof(T)));
		}
	};
}
