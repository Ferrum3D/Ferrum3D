#pragma once
#include <FeCore/Base/Base.h>

#if FE_WINDOWS
#    define FE_PLATFORM_ATOMIC_INT32 volatile long
#    define FE_PLATFORM_ATOMIC_INT64 volatile ::FE::Int64
#else
#endif

namespace FE
{
    /**
     * @brief Platform-specific atomic 32-bit integer.
    */
    using AtomicInt32 = FE_PLATFORM_ATOMIC_INT32;

    /**
     * @brief Platform-specific atomic 64-bit integer.
    */
    using AtomicInt64 = FE_PLATFORM_ATOMIC_INT64;

    static_assert(sizeof(AtomicInt32) == sizeof(Int32));
    static_assert(sizeof(AtomicInt64) == sizeof(Int64));

    /**
     * @brief Platform-specific atomics.
    */
    struct Interlocked
    {
        //=========================================================================================
        // ADD

        /**
         * @brief Add two 32-bit integers.
         * @param dst Destination.
         * @param val Second operand.
         * @return The new value.
        */
        static Int32 Add(AtomicInt32& dst, Int32 val);

        /**
         * @brief Add two 64-bit integers.
         * @param dst Destination.
         * @param val Second operand.
         * @return The new value.
        */
        static Int64 Add(AtomicInt64& dst, Int64 val);

        //=========================================================================================
        // AND

        /**
         * @brief Bitwise AND two 32-bit signed integers.
         * @param dst Destination.
         * @param val Second operand.
         * @return The original value at dst.
        */
        static Int32 And(AtomicInt32& dst, Int32 val);

        /**
         * @brief Bitwise AND two 64-bit signed integers.
         * @param dst Destination.
         * @param val Second operand.
         * @return The original value at dst.
        */
        static Int64 And(AtomicInt64& dst, Int64 val);

        //=========================================================================================
        // OR

        /**
         * @brief Bitwise OR two 32-bit signed integers.
         * @param dst Destination.
         * @param val Second operand.
         * @return The original value at dst.
        */
        static Int32 Or(AtomicInt32& dst, Int32 val);

        /**
         * @brief Bitwise OR two 64-bit signed integers.
         * @param dst Destination.
         * @param val Second operand.
         * @return The original value at dst.
        */
        static Int64 Or(AtomicInt64& dst, Int64 val);

        //=========================================================================================
        // Exchange

        /**
         * @brief Set a 32-bit integer to a specified value.
         * @param dst Destination
         * @param val The new value.
         * @return The original value at dst.
        */
        static Int32 Exchange(AtomicInt32& dst, Int32 val);

        /**
         * @brief Set a 64-bit integer to a specified value.
         * @param dst Destination
         * @param val The new value.
         * @return The original value at dst.
        */
        static Int64 Exchange(AtomicInt64& dst, Int64 val);

        //=========================================================================================
        // Increment

        /**
         * @brief Increment a 32-bit integer and store the result.
         * @param val The variable to increment.
         * @return The new value.
        */
        static Int32 Increment(AtomicInt32& val);

        /**
         * @brief Increment a 64-bit integer and store the result.
         * @param val The variable to increment.
         * @return The new value.
        */
        static Int64 Increment(AtomicInt64& val);

        //=========================================================================================
        // Decrement

        /**
         * @brief Decrement a 32-bit integer and store the result.
         * @param val The variable to decrement.
         * @return The new value.
        */
        static Int32 Decrement(AtomicInt32& val);

        /**
         * @brief Decrement a 64-bit integer and store the result.
         * @param val The variable to decrement.
         * @return The new value.
        */
        static Int64 Decrement(AtomicInt64& val);

        //=========================================================================================
        // CompareExchange

        /**
         * @brief Compare dst and cmp and, if equal, replace dst with exchg.
         * @param dst Destination, possibly replaced.
         * @param exchg The value that replaces the dst if dst was equal to cmp.
         * @param cmp The value that is compared to dst.
         * @return The original value at dst.
        */
        static Int32 CompareExchange(AtomicInt32& dst, Int32 exchg, Int32 cmp);

        /**
         * @brief Compare dst and cmp and, if equal, replace dst with exchg.
         * @param dst Destination, possibly replaced.
         * @param exchg The value that replaces the dst if dst was equal to cmp.
         * @param cmp The value that is compared to dst.
         * @return The original value at dst.
        */
        static Int64 CompareExchange(AtomicInt64& dst, Int64 exchg, Int64 cmp);
    };
} // namespace FE
