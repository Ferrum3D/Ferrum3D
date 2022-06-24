#pragma once
#include <FeCore/Base/Base.h>

#if FE_WINDOWS
#    define FE_PLATFORM_ATOMIC_INT16 volatile short
#    define FE_PLATFORM_ATOMIC_INT32 volatile long
#    define FE_PLATFORM_ATOMIC_INT64 volatile ::FE::Int64
#else
#    error Platform not supported yet
#endif

namespace FE
{
    //! \brief Platform-specific atomic 16-bit integer.
    using AtomicInt16 = FE_PLATFORM_ATOMIC_INT16;

    //! \brief Platform-specific atomic 32-bit integer.
    using AtomicInt32 = FE_PLATFORM_ATOMIC_INT32;

    //! \brief Platform-specific atomic 64-bit integer.
    using AtomicInt64 = FE_PLATFORM_ATOMIC_INT64;

    static_assert(sizeof(AtomicInt16) == sizeof(Int16));
    static_assert(sizeof(AtomicInt32) == sizeof(Int32));
    static_assert(sizeof(AtomicInt64) == sizeof(Int64));

    //! \brief Platform-specific atomics.
    struct Interlocked
    {
        //=========================================================================================
        // LOAD

        //! \brief Convert AtomicInt32 to Int32.
        //!
        //! \param [in] src - Value to load.
        //!
        //! \return The loaded value.
        FE_FINLINE static Int16 Load(const AtomicInt16& src);

        //! \brief Convert AtomicInt32 to Int32.
        //!
        //! \param [in] src - Value to load.
        //!
        //! \return The loaded value.
        FE_FINLINE static Int32 Load(const AtomicInt32& src);

        //! \brief Convert AtomicInt64 to Int64.
        //!
        //! \param [in] src - Value to load.
        //!
        //! \return The loaded value.
        FE_FINLINE static Int64 Load(const AtomicInt64& src);

        //=========================================================================================
        // ADD

        //! \brief Add two 32-bit integers.
        //! 
        //! \param [in] dst - Destination.
        //! \param [in] val - Second operand.
        //! 
        //! \return The new value.
        static Int32 Add(AtomicInt32& dst, Int32 val);

        //! \brief Add two 64-bit integers.
        //! 
        //! \param [out] dst - Destination.
        //! \param [in] val  - Second operand.
        //! 
        //! \return The new value.
        static Int64 Add(AtomicInt64& dst, Int64 val);

        //=========================================================================================
        // AND

        //! \brief Bitwise AND two 16-bit signed integers.
        //!
        //! \param [out] dst - Destination.
        //! \param [in] val  - Second operand.
        //!
        //! \return The original value at dst.
        static Int16 And(AtomicInt16& dst, Int16 val);

        //! \brief Bitwise AND two 32-bit signed integers.
        //! 
        //! \param [out] dst - Destination.
        //! \param [in] val  - Second operand.
        //! 
        //! \return The original value at dst.
        static Int32 And(AtomicInt32& dst, Int32 val);

        //! \brief Bitwise AND two 64-bit signed integers.
        //! 
        //! \param [out] dst - Destination.
        //! \param [in] val  - Second operand.
        //! 
        //! \return The original value at dst.
        static Int64 And(AtomicInt64& dst, Int64 val);

        //=========================================================================================
        // OR

        //! \brief Bitwise OR two 16-bit signed integers.
        //!
        //! \param [out] dst - Destination.
        //! \param [in] val  - Second operand.
        //!
        //! \return The original value at dst.
        static Int16 Or(AtomicInt16& dst, Int16 val);

        //! \brief Bitwise OR two 32-bit signed integers.
        //! 
        //! \param [out] dst - Destination.
        //! \param [in] val  - Second operand.
        //! 
        //! \return The original value at dst.
        static Int32 Or(AtomicInt32& dst, Int32 val);

        //! \brief Bitwise OR two 64-bit signed integers.
        //! 
        //! \param [out] dst - Destination.
        //! \param [in] val  - Second operand.
        //! 
        //! \return The original value at dst.
        static Int64 Or(AtomicInt64& dst, Int64 val);

        //=========================================================================================
        // Exchange

        //! \brief Set a 16-bit integer to a specified value.
        //!
        //! \param [out] dst - Destination
        //! \param [in] val  - The new value.
        //!
        //! \return The original value at dst.
        static Int16 Exchange(AtomicInt16& dst, Int16 val);

        //! \brief Set a 32-bit integer to a specified value.
        //! 
        //! \param [out] dst - Destination
        //! \param [in] val  - The new value.
        //! 
        //! \return The original value at dst.
        static Int32 Exchange(AtomicInt32& dst, Int32 val);

        //! \brief Set a 64-bit integer to a specified value.
        //! 
        //! \param [out] dst - Destination
        //! \param [in] val  - The new value.
        //! 
        //! \return The original value at dst.
        static Int64 Exchange(AtomicInt64& dst, Int64 val);

        //=========================================================================================
        // Increment

        //! \brief Increment a 16-bit integer and store the result.
        //!
        //! \param [out] val - The variable to increment.
        //!
        //! \return The new value.
        static Int16 Increment(AtomicInt16& val);

        //! \brief Increment a 32-bit integer and store the result.
        //! 
        //! \param [out] val - The variable to increment.
        //! 
        //! \return The new value.
        static Int32 Increment(AtomicInt32& val);

        //! \brief Increment a 64-bit integer and store the result.
        //! 
        //! \param [out] val - The variable to increment.
        //! 
        //! \return The new value.
        static Int64 Increment(AtomicInt64& val);

        //=========================================================================================
        // Decrement

        //! \brief Decrement a 16-bit integer and store the result.
        //!
        //! \param [out] val - The variable to decrement.
        //!
        //! \return The new value.
        static Int16 Decrement(AtomicInt16& val);

        //! \brief Decrement a 32-bit integer and store the result.
        //! 
        //! \param [out] val - The variable to decrement.
        //! 
        //! \return The new value.
        static Int32 Decrement(AtomicInt32& val);

        //! \brief Decrement a 64-bit integer and store the result.
        //! 
        //! \param [out] val - The variable to decrement.
        //! 
        //! \return The new value.
        static Int64 Decrement(AtomicInt64& val);

        //=========================================================================================
        // CompareExchange

        //! \brief Compare dst and cmp and, if equal, replace dst with exchg.
        //!
        //! \param [out] dst  - Destination, possibly replaced.
        //! \param [in] exchg - The value that replaces the dst if dst was equal to cmp.
        //! \param [in] cmp   - The value that is compared to dst.
        //!
        //! \return The original value at dst.
        static Int16 CompareExchange(AtomicInt16& dst, Int16 exchg, Int16 cmp);

        //! \brief Compare dst and cmp and, if equal, replace dst with exchg.
        //! 
        //! \param [out] dst  - Destination, possibly replaced.
        //! \param [in] exchg - The value that replaces the dst if dst was equal to cmp.
        //! \param [in] cmp   - The value that is compared to dst.
        //! 
        //! \return The original value at dst.
        static Int32 CompareExchange(AtomicInt32& dst, Int32 exchg, Int32 cmp);

        //! \brief Compare dst and cmp and, if equal, replace dst with exchg.
        //! 
        //! \param [out] dst  - Destination, possibly replaced.
        //! \param [in] exchg - The value that replaces the dst if dst was equal to cmp.
        //! \param [in] cmp   - The value that is compared to dst.
        //! 
        //! \return The original value at dst.
        static Int64 CompareExchange(AtomicInt64& dst, Int64 exchg, Int64 cmp);
    };

    Int16 Interlocked::Load(const AtomicInt16& src)
    {
        return static_cast<Int16>(src);
    }

    Int32 Interlocked::Load(const AtomicInt32& src)
    {
        return static_cast<Int32>(src);
    }

    Int64 Interlocked::Load(const AtomicInt64& src)
    {
        return static_cast<Int64>(src);
    }
} // namespace FE
