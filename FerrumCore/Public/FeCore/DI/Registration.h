#pragma once
#include <FeCore/DI/BaseDI.h>

namespace FE::DI
{
    struct ServiceRegistration final
    {
        ServiceRegistration() = default;

        [[nodiscard]] uint32_t GetIndex() const
        {
            return m_index;
        }

        Lifetime GetLifetime() const
        {
            return static_cast<Lifetime>(m_lifetime);
        }

        void SetLifetime(Lifetime lifetime)
        {
            m_lifetime = festd::to_underlying(lifetime);
        }

        void SetConstant(bool value)
        {
            m_constant = value;
        }

        bool IsConstant() const
        {
            return m_constant;
        }

        void SetFunction(bool value)
        {
            m_function = value;
        }

        bool IsFunction() const
        {
            return m_function;
        }

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

        static constexpr uint32_t LifetimeBits = 2;
        static_assert((1 << LifetimeBits) >= festd::to_underlying(Lifetime::kCount));

        uint32_t m_index : 28;
        uint32_t m_lifetime : LifetimeBits;
        uint32_t m_constant : 1;
        uint32_t m_function : 1;

        ServiceRegistration(uint32_t index)
            : m_index(index)
            , m_lifetime(0)
            , m_constant(false)
            , m_function(false)
        {
            FE_CORE_ASSERT(index < (1 << 28), "Too many services in one registry");
        }
    };

    static_assert(sizeof(ServiceRegistration) == sizeof(uint32_t));
} // namespace FE::DI

template<>
struct eastl::hash<FE::DI::ServiceRegistration>
{
    uint64_t operator()(FE::DI::ServiceRegistration value) const
    {
        return FE::DefaultHash(&value, sizeof(value));
    }
};
