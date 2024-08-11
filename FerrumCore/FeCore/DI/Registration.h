#pragma once
#include <FeCore/DI/BaseDI.h>

namespace FE::DI
{
    class ServiceRegistration final
    {
        friend class ServiceRegistry;

        inline static constexpr uint32_t LifetimeBits = 2;
        static_assert((1 << LifetimeBits) >= enum_cast(Lifetime::Count));

        uint32_t m_Index : 28;
        uint32_t m_Lifetime : LifetimeBits;
        uint32_t m_Constant : 1;
        uint32_t m_Function : 1;

        inline ServiceRegistration(uint32_t index)
            : m_Index(index)
            , m_Lifetime(0)
            , m_Constant(false)
            , m_Function(false)
        {
            FE_CORE_ASSERT(index < (1 << 28), "Too many services in one registry");
        }

    public:
        inline ServiceRegistration() = default;

        inline uint32_t GetIndex() const
        {
            return m_Index;
        }

        inline Lifetime GetLifetime() const
        {
            return static_cast<Lifetime>(m_Lifetime);
        }

        inline void SetLifetime(Lifetime lifetime)
        {
            m_Lifetime = enum_cast(lifetime);
        }

        inline void SetConstant(bool value)
        {
            m_Constant = value;
        }

        inline bool IsConstant() const
        {
            return m_Constant;
        }

        inline void SetFunction(bool value)
        {
            m_Function = value;
        }

        inline bool IsFunction() const
        {
            return m_Function;
        }

        inline friend bool operator==(const ServiceRegistration& lhs, const ServiceRegistration& rhs)
        {
            return bit_cast<uint32_t>(lhs) == bit_cast<uint32_t>(rhs);
        }

        inline friend bool operator!=(const ServiceRegistration& lhs, const ServiceRegistration& rhs)
        {
            return bit_cast<uint32_t>(lhs) != bit_cast<uint32_t>(rhs);
        }
    };

    static_assert(sizeof(ServiceRegistration) == sizeof(uint32_t));
} // namespace FE::DI

template<>
struct eastl::hash<FE::DI::ServiceRegistration>
{
    inline uint64_t operator()(FE::DI::ServiceRegistration value) const
    {
        return FE::DefaultHash(&value, sizeof(value));
    }
};
