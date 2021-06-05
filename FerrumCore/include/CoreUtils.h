#pragma once
#include <atomic>
#include "Platform.h"

namespace Ferrum
{
	/**
	* @brief Name of the engine
	*/
	inline constexpr const char* FerrumEngineName = "Ferrum3D";

	/**
	* @brief Engine version
	*/
	inline constexpr struct
	{
		int Major = 0, Minor = 1, Patch = 0;
	} Ferrum3DVersion;

	template<class T, class Func>
	inline void FeAtomicBitOp(std::atomic<T>& a, uint8_t n, Func bitOp) {
		static_assert(std::is_integral<T>(), "T must be integral");

		T val = a.load(), res = bitOp(val, n);
		while (!a.compare_exchange_weak(val, res));
	}

	inline constexpr auto FeSetBit = [](auto val, unsigned n) { return val | (1 << n); };
	inline constexpr auto FeResetBit = [](auto val, unsigned n) { return val & ~(1 << n); };
	inline constexpr auto FeXorBit = [](auto val, unsigned n) { return val ^ (1 << n); };
}
