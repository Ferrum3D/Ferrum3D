#pragma once
#include <atomic>
#include <cstdint>
#include <intrin.h>
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

	template<class T>
	inline constexpr T FeMakeAlignment(T x, T align) {
		return (x + a - 1u) & ~(a - 1u);
	}

	template<uint32_t A, class T>
	inline constexpr T FeMakeAlignment(T x) {
		return (x + A - 1u) & ~(A - 1u);
	}

	inline constexpr uint32_t FeNextPowerOf2(uint32_t v) {
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		return v;
	}

	inline constexpr auto FeSetBit = [](auto val, unsigned n) { return val | (1 << n); };
	inline constexpr auto FeResetBit = [](auto val, unsigned n) { return val & ~(1 << n); };
	inline constexpr auto FeXorBit = [](auto val, unsigned n) { return val ^ (1 << n); };

#ifdef _MSC_VER

	uint32_t inline FeCountTrailingZeros(uint32_t value)
	{
		unsigned long tz = 0;
		return _BitScanForward(&tz, value) ? tz : 32;
	}

	uint32_t inline FeCountLeadingZeros(uint32_t value)
	{
		unsigned long lz = 0;
		return _BitScanReverse(&lz, value) ? 31 - lz : 32;
	}

#else

	uint32_t inline FeCountTrailingZeros(uint32_t value)
	{
		return __builtin_ctz(value);
	}

	uint32_t inline FeCountLeadingZeros(uint32_t value)
	{
		return __builtin_clz(value);
	}

#endif

#define FE_TYPED_ENUM(_name, _type)																	\
	enum class _name : _type;																		\
	inline _name operator|(_name a, _name b) { return _name(_type(a) | _type(b)); }					\
	inline _name operator&(_name a, _name b) { return _name(_type(a) | _type(b)); }					\
	inline _name& operator|=(_name& a, _name b) { return a = a | b; }								\
	inline _name& operator&=(_name& a, _name b) { return a = a & b; }								\
																									\
	inline _name operator|(_name a, _type b) { return _name(_type(a) | b); }						\
	inline _name operator&(_name a, _type b) { return _name(_type(a) | b); }						\
	inline _name& operator|=(_name& a, _type b) { return a = a | b; }								\
	inline _name& operator&=(_name& a, _type b) { return a = a & b; }								\
																									\
	enum class _name : _type

#define FE_ENUM(_name) FE_TYPED_ENUM(_name, int)
}
