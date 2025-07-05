#pragma once
#include <FeCore/DI/BaseDI.h>

namespace FE::DI
{
    struct ServiceRegistration final
    {
        ServiceRegistration() = default;

        static constexpr uint32_t kLifetimeBits = 2;
        static constexpr uint32_t kMaxLifetimeCount = 1 << kLifetimeBits;
        static_assert(kMaxLifetimeCount >= festd::to_underlying(Lifetime::kCount));

        static constexpr uint32_t kIndexBits = 32 - kLifetimeBits - 2;
        static constexpr uint32_t kMaxIndex = (1 << kIndexBits) - 1;

        uint32_t m_index : kIndexBits;
        Lifetime m_lifetime : kLifetimeBits;
        uint32_t m_isConstant : 1;
        uint32_t m_isFunction : 1;

        friend bool operator==(const ServiceRegistration& lhs, const ServiceRegistration& rhs)
        {
            return festd::bit_cast<uint32_t>(lhs) == festd::bit_cast<uint32_t>(rhs);
        }

        friend bool operator!=(const ServiceRegistration& lhs, const ServiceRegistration& rhs)
        {
            return festd::bit_cast<uint32_t>(lhs) != festd::bit_cast<uint32_t>(rhs);
        }

    private:
        friend struct ServiceRegistry;

        ServiceRegistration(const uint32_t index)
            : m_index(index)
            , m_lifetime(Lifetime::kSingleton)
            , m_isConstant(false)
            , m_isFunction(false)
        {
            FE_Assert(index <= kMaxIndex, "Too many services in one registry");
        }
    };

    static_assert(sizeof(ServiceRegistration) == sizeof(uint32_t));
} // namespace FE::DI


template<>
struct eastl::hash<FE::DI::ServiceRegistration>
{
    uint64_t operator()(const FE::DI::ServiceRegistration value) const
    {
        return FE::DefaultHash(&value, sizeof(value));
    }
};
