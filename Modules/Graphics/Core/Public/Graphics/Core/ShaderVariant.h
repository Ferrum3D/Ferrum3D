#pragma once
#include <FeCore/Base/Assert.h>

namespace FE::Graphics::Core
{
    enum class ShaderSpecializationType : uint32_t
    {
        kNone,
        kSpecializationConstant,
        kPreprocessorDefine,
        kCount,
    };


    namespace Internal
    {
        template<class T>
        constexpr T IncrementIntLike(const T value)
        {
            if constexpr (std::is_enum_v<T>)
                return static_cast<T>(static_cast<std::underlying_type_t<T>>(value) + 1);
            else
                return value + 1;
        }


        template<ShaderSpecializationType TSpecializationType, class T, T... TValues>
        struct ShaderSpecializationBase
        {
            using ValueType = T;
            static constexpr auto kSpecializationType = TSpecializationType;
            static constexpr uint32_t kVariantCount = sizeof...(TValues);

            static constexpr T GetValueByVariantIndex(const uint32_t index)
            {
                constexpr T values[] = { TValues... };
                if (index >= kVariantCount)
                    return T{};

                return values[index];
            }

            static constexpr uint32_t GetVariantIndexByValue(const T value)
            {
                constexpr T values[] = { TValues... };
                for (uint32_t variantIndex = 0; variantIndex < kVariantCount; ++variantIndex)
                {
                    if (values[variantIndex] == value)
                        return variantIndex;
                }

                return kInvalidIndex;
            }
        };


        template<ShaderSpecializationType TSpecializationType, class T, T TIndex, T TSize, T... TValues>
        struct ShaderSpecializationIntHelper final
        {
            using Type = typename ShaderSpecializationIntHelper<TSpecializationType, T, IncrementIntLike(TIndex), TSize,
                                                                TValues..., TIndex>::Type;
        };


        template<ShaderSpecializationType TSpecializationType, class T, T TIndex, T... TValues>
        struct ShaderSpecializationIntHelper<TSpecializationType, T, TIndex, TIndex, TValues...> final
        {
            using Type = ShaderSpecializationBase<TSpecializationType, T, TValues...>;
        };


        template<ShaderSpecializationType TSpecializationType>
        using ShaderSpecializationBool = ShaderSpecializationBase<TSpecializationType, bool, false, true>;


        template<ShaderSpecializationType TSpecializationType, auto TSize>
        using ShaderSpecializationInt =
            typename ShaderSpecializationIntHelper<TSpecializationType, decltype(TSize), 0, TSize>::Type;


        template<ShaderSpecializationType TSpecializationType, class TEnum>
        using ShaderSpecializationEnum =
            typename ShaderSpecializationIntHelper<TSpecializationType, TEnum, static_cast<TEnum>(0), TEnum::kCount>::Type;
    } // namespace Internal


#define FE_SHADER_SPEC(name, value, ...)                                                                                         \
    struct name final                                                                                                            \
        : public FE::Graphics::Core::Internal::                                                                                  \
              ShaderSpecializationBase<FE::Graphics::Core::ShaderSpecializationType::kNone, decltype(value), value, __VA_ARGS__> \
    {                                                                                                                            \
        static constexpr const char* kName = #name;                                                                              \
    }


#define FE_SHADER_SPEC_CONSTANT(name, value, ...)                                                                                \
    struct name final                                                                                                            \
        : public FE::Graphics::Core::Internal::ShaderSpecializationBase<                                                         \
              FE::Graphics::Core::ShaderSpecializationType::kSpecializationConstant,                                             \
              decltype(value),                                                                                                   \
              value,                                                                                                             \
              __VA_ARGS__>                                                                                                       \
    {                                                                                                                            \
        static constexpr const char* kName = #name;                                                                              \
    }


#define FE_SHADER_SPEC_DEFINE(name, def, value, ...)                                                                             \
    struct name final                                                                                                            \
        : public FE::Graphics::Core::Internal::ShaderSpecializationBase<                                                         \
              FE::Graphics::Core::ShaderSpecializationType::kPreprocessorDefine,                                                 \
              decltype(value),                                                                                                   \
              value,                                                                                                             \
              __VA_ARGS__>                                                                                                       \
    {                                                                                                                            \
        static constexpr const char* kName = def;                                                                                \
    }


#define FE_SHADER_SPEC_RANGE(name, size)                                                                                         \
    struct name final                                                                                                            \
        : public FE::Graphics::Core::Internal::ShaderSpecializationInt<FE::Graphics::Core::ShaderSpecializationType::kNone,      \
                                                                       size>                                                     \
    {                                                                                                                            \
        static constexpr const char* kName = #name;                                                                              \
    }


#define FE_SHADER_SPEC_CONSTANT_RANGE(name, size)                                                                                \
    struct name final                                                                                                            \
        : public FE::Graphics::Core::Internal::                                                                                  \
              ShaderSpecializationInt<FE::Graphics::Core::ShaderSpecializationType::kSpecializationConstant, size>               \
    {                                                                                                                            \
        static constexpr const char* kName = #name;                                                                              \
    }


#define FE_SHADER_SPEC_DEFINE_RANGE(name, def, size)                                                                             \
    struct name final                                                                                                            \
        : public FE::Graphics::Core::Internal::                                                                                  \
              ShaderSpecializationInt<FE::Graphics::Core::ShaderSpecializationType::kPreprocessorDefine, size>                   \
    {                                                                                                                            \
        static constexpr const char* kName = def;                                                                                \
    }


#define FE_SHADER_SPEC_ENUM(name, enumType)                                                                                      \
    struct name final                                                                                                            \
        : public FE::Graphics::Core::Internal::ShaderSpecializationEnum<FE::Graphics::Core::ShaderSpecializationType::kNone,     \
                                                                        enumType>                                                \
    {                                                                                                                            \
        static constexpr const char* kName = #name;                                                                              \
    }


#define FE_SHADER_SPEC_CONSTANT_ENUM(name, enumType)                                                                             \
    struct name final                                                                                                            \
        : public FE::Graphics::Core::Internal::                                                                                  \
              ShaderSpecializationEnum<FE::Graphics::Core::ShaderSpecializationType::kSpecializationConstant, enumType>          \
    {                                                                                                                            \
        static constexpr const char* kName = #name;                                                                              \
    }


#define FE_SHADER_SPEC_DEFINE_ENUM(name, def, enumType)                                                                          \
    struct name final                                                                                                            \
        : public FE::Graphics::Core::Internal::                                                                                  \
              ShaderSpecializationEnum<FE::Graphics::Core::ShaderSpecializationType::kPreprocessorDefine, enumType>              \
    {                                                                                                                            \
        static constexpr const char* kName = def;                                                                                \
    }


#define FE_SHADER_SPEC_BOOL(name)                                                                                                \
    struct name final                                                                                                            \
        : public FE::Graphics::Core::Internal::ShaderSpecializationBool<FE::Graphics::Core::ShaderSpecializationType::kNone>     \
    {                                                                                                                            \
        static constexpr const char* kName = #name;                                                                              \
    }


#define FE_SHADER_SPEC_CONSTANT_BOOL(name)                                                                                       \
    struct name final                                                                                                            \
        : public FE::Graphics::Core::Internal::ShaderSpecializationBool<                                                         \
              FE::Graphics::Core::ShaderSpecializationType::kSpecializationConstant>                                             \
    {                                                                                                                            \
        static constexpr const char* kName = #name;                                                                              \
    }


#define FE_SHADER_SPEC_DEFINE_BOOL(name, def)                                                                                    \
    struct name final                                                                                                            \
        : public FE::Graphics::Core::Internal::ShaderSpecializationBool<                                                         \
              FE::Graphics::Core::ShaderSpecializationType::kPreprocessorDefine>                                                 \
    {                                                                                                                            \
        static constexpr const char* kName = def;                                                                                \
    }


    template<class... TSpecializations>
    struct ShaderSpecializer;


    template<>
    struct ShaderSpecializer<> final
    {
        static constexpr uint32_t kVariantCount = 1;

        ShaderSpecializer() = default;

        explicit ShaderSpecializer(const uint32_t variantIndex)
        {
            FE_Assert(variantIndex == 0);
        }

        [[nodiscard]] uint32_t GetVariantIndex() const
        {
            return 0;
        }

        bool operator==(const ShaderSpecializer&) const
        {
            return true;
        }

        bool operator!=(const ShaderSpecializer&) const
        {
            return false;
        }

    private:
        template<class... TSpecializations>
        friend struct ShaderSpecializer;

        void AddDefines([[maybe_unused]] festd::pmr::vector<ShaderDefine>& defines,
                        [[maybe_unused]] std::pmr::memory_resource* tempAllocator) const
        {
        }

        void AddSpecializationConstants(
            [[maybe_unused]] festd::pmr::vector<ShaderSpecializationConstant>& specializationConstants) const
        {
        }
    };


    template<class TSpecialization, class... TOtherSpecializations>
    struct ShaderSpecializer<TSpecialization, TOtherSpecializations...> final
    {
        using ValueType = typename TSpecialization::ValueType;
        static constexpr uint32_t kVariantCount =
            TSpecialization::kVariantCount * ShaderSpecializer<TOtherSpecializations...>::kVariantCount;

        ShaderSpecializer()
            : m_value(TSpecialization::GetValueByVariantIndex(0))
        {
        }

        explicit ShaderSpecializer(const uint32_t variantIndex)
            : m_value(TSpecialization::GetValueByVariantIndex(variantIndex % TSpecialization::kVariantCount))
            , m_parent(variantIndex / TSpecialization::kVariantCount)
        {
            FE_Assert(variantIndex < kVariantCount);
        }

        template<class TSpec>
        [[nodiscard]] ValueType Get() const
        {
            if constexpr (std::is_same_v<TSpec, TSpecialization>)
                return m_value;
            else
                return m_parent.template Get<TSpec>();
        }

        template<class TSpec>
        void Set(const typename TSpec::ValueType value)
        {
            if constexpr (std::is_same_v<TSpec, TSpecialization>)
                m_value = value;
            else
                m_parent.template Set<TSpec>(value);
        }

        [[nodiscard]] uint32_t GetVariantIndex() const
        {
            return TSpecialization::GetVariantIndexByValue(m_value) + m_parent.GetVariantIndex() * TSpecialization::kVariantCount;
        }

        Env::Name GetDefines(std::pmr::memory_resource* temp) const
        {
            FE_AssertDebug(temp != std::pmr::get_default_resource(), "This function does not free memory");

            festd::pmr::vector<ShaderDefine> defines{ temp };
            defines.reserve(sizeof...(TOtherSpecializations) + 1);
            AddDefines(defines, temp);
            return CombineDefines(defines);
        }

        festd::pmr::vector<ShaderSpecializationConstant> GetSpecializationConstants(std::pmr::memory_resource* temp) const
        {
            festd::pmr::vector<ShaderSpecializationConstant> specializationConstants{ temp };
            specializationConstants.reserve(sizeof...(TOtherSpecializations) + 1);
            AddSpecializationConstants(specializationConstants);
            return specializationConstants;
        }

        bool operator==(const ShaderSpecializer& other) const
        {
            return m_value == other.m_value && m_parent == other.m_parent;
        }

        bool operator!=(const ShaderSpecializer& other) const
        {
            return !(*this == other);
        }

    private:
        template<class... TSpecializations>
        friend struct ShaderSpecializer;

        ValueType m_value;
        ShaderSpecializer<TOtherSpecializations...> m_parent;

        void AddDefines(festd::pmr::vector<ShaderDefine>& defines, std::pmr::memory_resource* tempAllocator) const
        {
            if (TSpecialization::kSpecializationType == ShaderSpecializationType::kPreprocessorDefine)
            {
                int32_t value;
                if constexpr (std::is_enum_v<ValueType>)
                {
                    value = festd::to_underlying(m_value);
                }
                else
                {
                    value = static_cast<int32_t>(m_value);
                }

                const festd::string_view valueString = Str::Duplicate(Fmt::FixedFormat("{}", value), tempAllocator);
                defines.push_back({ TSpecialization::kName, valueString });
            }

            m_parent.AddDefines(defines, tempAllocator);
        }

        void AddSpecializationConstants(festd::pmr::vector<ShaderSpecializationConstant>& constants) const
        {
            if (TSpecialization::kSpecializationType == ShaderSpecializationType::kSpecializationConstant)
            {
                constants.push_back({ Env::Name{ TSpecialization::kName }, static_cast<int32_t>(m_value) });
            }

            m_parent.AddSpecializationConstants(constants);
        }
    };
} // namespace FE::Graphics::Core
