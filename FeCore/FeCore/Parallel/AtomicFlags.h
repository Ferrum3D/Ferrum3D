#pragma once
#include <FeCore/Parallel/Interlocked.h>
#include <FeCore/RTTI/RTTI.h>

namespace FE
{
    template<class TFlags>
    struct AtomicFlags
    {
        using AtomicType = std::conditional_t<sizeof(TFlags) == 64, AtomicInt64, AtomicInt32>;
        using IntegralType = std::conditional_t<sizeof(TFlags) == 64, Int64, Int32>;

        AtomicType Data = 0;

        FE_STRUCT_RTTI(AtomicFlags, "07644FF7-CC29-4277-AA81-F68321231826");

        FE_FINLINE AtomicFlags(TFlags flags) // NOLINT
        {
            Interlocked::Exchange(Data, static_cast<IntegralType>(flags));
        }

        FE_FINLINE AtomicFlags(const AtomicFlags& other) noexcept
        {
            Interlocked::Exchange(Data, Interlocked::Load(other.Data));
        }

        FE_FINLINE AtomicFlags(AtomicFlags&& other) noexcept
        {
            Interlocked::Exchange(Data, Interlocked::Load(other.Data));
        }

        [[nodiscard]] FE_FINLINE TFlags GetFlags() const
        {
            return static_cast<TFlags>(Interlocked::Load(Data));
        }

        FE_FINLINE AtomicFlags& operator=(const AtomicFlags& other) noexcept
        {
            Interlocked::Exchange(Data, Interlocked::Load(other.Data));
            return *this;
        }

        FE_FINLINE AtomicFlags& operator=(AtomicFlags&& other) noexcept
        {
            Interlocked::Exchange(Data, Interlocked::Load(other.Data));
            return *this;
        }

        FE_FINLINE AtomicFlags operator|(AtomicFlags rhs) const
        {
            return Interlocked::Load(Data) | Interlocked::Load(rhs.Data);
        }

        FE_FINLINE AtomicFlags& operator|=(AtomicFlags rhs) const
        {
            Interlocked::Exchange(Data, Interlocked::Load(Data) | Interlocked::Load(rhs.Data));
            return *this;
        }

        FE_FINLINE AtomicFlags operator&(AtomicFlags rhs) const
        {
            return Interlocked::Load(Data) & Interlocked::Load(rhs.Data);
        }

        FE_FINLINE AtomicFlags& operator&=(AtomicFlags rhs) const
        {
            Interlocked::Exchange(Data, Interlocked::Load(Data) & Interlocked::Load(rhs.Data));
            return *this;
        }

        FE_FINLINE AtomicFlags operator^(AtomicFlags rhs) const
        {
            return Interlocked::Load(Data) & Interlocked::Load(rhs.Data);
        }

        FE_FINLINE AtomicFlags& operator^=(AtomicFlags rhs) const
        {
            Interlocked::Exchange(Data, Interlocked::Load(Data) & Interlocked::Load(rhs.Data));
            return *this;
        }

        FE_FINLINE friend bool operator==(const AtomicFlags& lhs, const AtomicFlags& rhs) noexcept
        {
            return Interlocked::Load(lhs.Data) == Interlocked::Load(rhs.Data);
        }

        FE_FINLINE friend bool operator!=(const AtomicFlags& lhs, const AtomicFlags& rhs) noexcept
        {
            return Interlocked::Load(lhs.Data) != Interlocked::Load(rhs.Data);
        }
    };
}
