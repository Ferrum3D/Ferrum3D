#pragma once
#include <Core/Base/Base.h>
#include <Core/IO/IStream.h>
#include <Core/RTTI/Reflection.h>
#include <concepts>
#include <festd/string.h>
#include <festd/vector.h>
#include <type_traits>

namespace FE::Serialization
{
    enum class Direction : uint8_t
    {
        kSerialize,
        kDeserialize,
    };


    enum class Format : uint8_t
    {
        kPackedBinary,
        kTaggedBinary,
        kJson,
    };


    enum class ScalarKind : uint8_t
    {
        kBool,
        kSigned,
        kUnsigned,
        kFloat,
    };


    template<class T, class = void>
    struct Serializer;


    class SerializationContext;


    namespace Internal
    {
        inline constexpr uint64_t kSchemaSeed = UINT64_C(0xf53e4f3d4f30ad27);


        constexpr uint64_t CombineSchemaHashes(const uint64_t lhs, const uint64_t rhs)
        {
            return FE::Internal::WyHash64(lhs ^ kSchemaSeed, rhs);
        }


        template<class T>
        using ValueType = std::remove_cv_t<std::remove_reference_t<T>>;
    } // namespace Internal


    template<class T>
    bool SerializeValue(SerializationContext& context, const T& value);


    template<class T>
    bool DeserializeValue(SerializationContext& context, T& value);


    template<class T>
    uint64_t GetSchemaHash();


    template<class T>
    uint32_t GetVersion();


    class SerializationContext
    {
    public:
        virtual ~SerializationContext() = default;

        SerializationContext(const SerializationContext&) = delete;
        SerializationContext(SerializationContext&&) = delete;
        SerializationContext& operator=(const SerializationContext&) = delete;
        SerializationContext& operator=(SerializationContext&&) = delete;

        [[nodiscard]] Direction GetDirection() const
        {
            return m_direction;
        }

        [[nodiscard]] Format GetFormat() const
        {
            return m_format;
        }

        [[nodiscard]] bool IsSerializing() const
        {
            return m_direction == Direction::kSerialize;
        }

        [[nodiscard]] bool IsDeserializing() const
        {
            return m_direction == Direction::kDeserialize;
        }

        [[nodiscard]] bool IsBinary() const
        {
            return m_format != Format::kJson;
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_isValid;
        }

        [[nodiscard]] uint32_t GetSerializedVersion() const
        {
            return m_serializedVersion;
        }

        [[nodiscard]] uint64_t GetSerializedSchemaHash() const
        {
            return m_serializedSchemaHash;
        }

        template<class T>
        bool Store(const T& value)
        {
            Begin(Direction::kSerialize);
            const Rtti::TypeID typeID = Rtti::GetTypeID<Internal::ValueType<T>>();
            if (!BeginDocument(typeID, GetVersion<T>(), GetSchemaHash<T>()))
                return false;

            if (!SerializeValue(*this, value))
                Fail();
            EndDocument();
            return m_isValid;
        }

        template<class T>
        bool Load(T& value)
        {
            Begin(Direction::kDeserialize);
            const Rtti::TypeID typeID = Rtti::GetTypeID<Internal::ValueType<T>>();
            if (!BeginDocument(typeID, GetVersion<T>(), GetSchemaHash<T>()))
                return false;

            if (!DeserializeValue(*this, value))
                Fail();
            EndDocument();
            return m_isValid;
        }

        bool Store(const Rtti::Type& type, const void* value);
        bool Load(const Rtti::Type& type, void* value);

        template<class T>
        SerializationContext& Field(const festd::ascii_view name, const T& value)
        {
            if (!m_isValid || !BeginField(name, CompileTimeHash(name.data(), name.size())))
                return *this;

            if (!SerializeValue(*this, value))
                Fail();
            EndField();
            return *this;
        }

        template<class T>
        SerializationContext& Field(const festd::ascii_view name, T& value)
        {
            if (!m_isValid || !BeginField(name, CompileTimeHash(name.data(), name.size())))
                return *this;

            if (IsSerializing())
            {
                if (!SerializeValue(*this, value))
                    Fail();
            }
            else
            {
                if (!DeserializeValue(*this, value))
                    Fail();
            }

            EndField();
            return *this;
        }

        template<class T>
        SerializationContext& Element(const uint32_t index, const T& value)
        {
            if (!m_isValid || !BeginElement(index))
                return *this;

            if (!SerializeValue(*this, value))
                Fail();
            EndElement();
            return *this;
        }

        template<class T>
        SerializationContext& Element(const uint32_t index, T& value)
        {
            if (!m_isValid || !BeginElement(index))
                return *this;

            if (IsSerializing())
            {
                if (!SerializeValue(*this, value))
                    Fail();
            }
            else
            {
                if (!DeserializeValue(*this, value))
                    Fail();
            }

            EndElement();
            return *this;
        }

        template<class T>
        SerializationContext& Number(T& value)
        {
            static_assert(std::is_arithmetic_v<T>);
            TransferScalar(GetScalarKind<T>(), &value, sizeof(T));
            return *this;
        }

        template<class T>
        SerializationContext& Number(const T& value)
        {
            static_assert(std::is_arithmetic_v<T>);
            FE_Assert(IsSerializing());
            TransferScalar(GetScalarKind<T>(), const_cast<T*>(&value), sizeof(T));
            return *this;
        }

        template<class T>
        SerializationContext& RawBytes(T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            TransferBytes(&value, sizeof(T));
            return *this;
        }

        template<class T>
        SerializationContext& RawBytes(const T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            FE_Assert(IsSerializing());
            TransferBytes(const_cast<T*>(&value), sizeof(T));
            return *this;
        }

        SerializationContext& String(festd::string_view value)
        {
            FE_Assert(IsSerializing());
            TransferString(nullptr, value);
            return *this;
        }

        SerializationContext& String(festd::string& value)
        {
            if (IsSerializing())
                TransferString(nullptr, value);
            else
                TransferString(&value, {});
            return *this;
        }

        bool BeginObject()
        {
            return m_isValid && BeginObjectImpl();
        }

        void EndObject()
        {
            if (m_isValid)
                EndObjectImpl();
        }

        bool BeginArray(uint32_t& size)
        {
            return m_isValid && BeginArrayImpl(size);
        }

        void EndArray()
        {
            if (m_isValid)
                EndArrayImpl();
        }

    protected:
        explicit SerializationContext(const Format format)
            : m_format(format)
        {
        }

        void Fail()
        {
            m_isValid = false;
        }

        virtual void Reset() = 0;
        virtual bool BeginDocument(Rtti::TypeID expectedType, uint32_t version, uint64_t schemaHash) = 0;
        virtual void EndDocument() = 0;
        virtual bool BeginObjectImpl() = 0;
        virtual void EndObjectImpl() = 0;
        virtual bool BeginField(festd::ascii_view name, uint64_t fieldID) = 0;
        virtual void EndField() = 0;
        virtual bool BeginArrayImpl(uint32_t& size) = 0;
        virtual void EndArrayImpl() = 0;
        virtual bool BeginElement(uint32_t index) = 0;
        virtual void EndElement() = 0;
        virtual void TransferScalar(ScalarKind kind, void* value, uint32_t byteSize) = 0;
        virtual void TransferBytes(void* value, uint32_t byteSize) = 0;
        virtual void TransferString(festd::string* output, festd::string_view input) = 0;

        Direction m_direction = Direction::kSerialize;
        const Format m_format;
        bool m_isValid = true;
        uint32_t m_serializedVersion = 0;
        uint64_t m_serializedSchemaHash = 0;

    private:
        void Begin(const Direction direction)
        {
            m_direction = direction;
            m_isValid = true;
            m_serializedVersion = 0;
            m_serializedSchemaHash = 0;
            Reset();
        }

        template<class T>
        static consteval ScalarKind GetScalarKind()
        {
            if constexpr (std::is_same_v<T, bool>)
                return ScalarKind::kBool;
            else if constexpr (std::is_floating_point_v<T>)
                return ScalarKind::kFloat;
            else if constexpr (std::is_signed_v<T>)
                return ScalarKind::kSigned;
            else
                return ScalarKind::kUnsigned;
        }
    };


    namespace Internal
    {
        template<class T>
        concept HasSerializer = requires(SerializationContext& context, const T& constValue, T& value) {
            { Serializer<T>::Serialize(context, constValue) } -> std::same_as<bool>;
            { Serializer<T>::Deserialize(context, value) } -> std::same_as<bool>;
        };


        template<class T>
        concept HasSerializerSchemaHash = requires {
            { Serializer<T>::GetSchemaHash() } -> std::convertible_to<uint64_t>;
        };


        template<class T>
        concept HasSerializerVersion = requires {
            { Serializer<T>::GetVersion() } -> std::convertible_to<uint32_t>;
        };
    } // namespace Internal


    template<class T>
    bool SerializeValue(SerializationContext& context, const T& value)
    {
        using ValueType = Internal::ValueType<T>;
        static_assert(Internal::HasSerializer<ValueType>, "No serializer is defined for this type");
        return Serializer<ValueType>::Serialize(context, value);
    }


    template<class T>
    bool DeserializeValue(SerializationContext& context, T& value)
    {
        using ValueType = Internal::ValueType<T>;
        static_assert(Internal::HasSerializer<ValueType>, "No serializer is defined for this type");
        return Serializer<ValueType>::Deserialize(context, value);
    }


    template<class T>
    uint64_t GetSchemaHash()
    {
        using ValueType = Internal::ValueType<T>;
        static_assert(Internal::HasSerializer<ValueType>, "No serializer is defined for this type");
        if constexpr (Internal::HasSerializerSchemaHash<ValueType>)
            return Serializer<ValueType>::GetSchemaHash();
        else
            return TypeNameHash<ValueType>;
    }


    template<class T>
    uint32_t GetVersion()
    {
        using ValueType = Internal::ValueType<T>;
        static_assert(Internal::HasSerializer<ValueType>, "No serializer is defined for this type");
        if constexpr (Internal::HasSerializerVersion<ValueType>)
            return Serializer<ValueType>::GetVersion();
        else if constexpr (requires { ValueType::kSerializationVersion; })
            return static_cast<uint32_t>(ValueType::kSerializationVersion);
        else if constexpr (requires { ValueType::kVersion; })
            return static_cast<uint32_t>(ValueType::kVersion);
        else
            return 0;
    }


    template<class T>
    struct Serializer<T, std::void_t<decltype(&T::RTTI_Serialize), decltype(&T::RTTI_Deserialize)>>
    {
        static bool Serialize(SerializationContext& context, const T& value)
        {
            return value.RTTI_Serialize(context);
        }

        static bool Deserialize(SerializationContext& context, T& value)
        {
            return value.RTTI_Deserialize(context);
        }

        static uint64_t GetSchemaHash()
        {
            return T::RTTI_GetSerializationSchemaHash();
        }

        static uint32_t GetVersion()
        {
            return T::RTTI_GetSerializationVersion();
        }
    };


    template<class T>
        requires std::is_arithmetic_v<T>
    struct Serializer<T>
    {
        static bool Serialize(SerializationContext& context, const T& value)
        {
            context.Number(value);
            return context.IsValid();
        }

        static bool Deserialize(SerializationContext& context, T& value)
        {
            context.Number(value);
            return context.IsValid();
        }

        static constexpr uint64_t GetSchemaHash()
        {
            return TypeNameHash<T>;
        }

        static constexpr uint32_t GetVersion()
        {
            return 0;
        }
    };


    template<class T>
        requires std::is_enum_v<T>
    struct Serializer<T>
    {
        using UnderlyingType = std::underlying_type_t<T>;

        static bool Serialize(SerializationContext& context, const T value)
        {
            const UnderlyingType underlyingValue = static_cast<UnderlyingType>(value);
            return SerializeValue(context, underlyingValue);
        }

        static bool Deserialize(SerializationContext& context, T& value)
        {
            UnderlyingType underlyingValue{};
            if (!DeserializeValue(context, underlyingValue))
                return false;

            value = static_cast<T>(underlyingValue);
            return true;
        }

        static constexpr uint64_t GetSchemaHash()
        {
            return Internal::CombineSchemaHashes(CompileTimeHash("enum", 4), TypeNameHash<T>);
        }

        static constexpr uint32_t GetVersion()
        {
            return 0;
        }
    };


    template<class T, size_t TSize>
    struct Serializer<T[TSize]>
    {
        static bool Serialize(SerializationContext& context, const T (&value)[TSize])
        {
            uint32_t size = static_cast<uint32_t>(TSize);
            if (!context.BeginArray(size))
                return false;

            for (uint32_t i = 0; i < size; ++i)
                context.Element(i, value[i]);
            context.EndArray();
            return context.IsValid();
        }

        static bool Deserialize(SerializationContext& context, T (&value)[TSize])
        {
            uint32_t size = 0;
            if (!context.BeginArray(size) || size != TSize)
                return false;

            for (uint32_t i = 0; i < size; ++i)
                context.Element(i, value[i]);
            context.EndArray();
            return context.IsValid();
        }

        static uint64_t GetSchemaHash()
        {
            return Internal::CombineSchemaHashes(Internal::CombineSchemaHashes(CompileTimeHash("array", 5), TSize),
                                                 Serialization::GetSchemaHash<T>());
        }

        static constexpr uint32_t GetVersion()
        {
            return 0;
        }
    };


    template<class T, size_t TSize>
    struct Serializer<eastl::array<T, TSize>>
    {
        static bool Serialize(SerializationContext& context, const eastl::array<T, TSize>& value)
        {
            uint32_t size = static_cast<uint32_t>(TSize);
            if (!context.BeginArray(size))
                return false;

            for (uint32_t i = 0; i < size; ++i)
                context.Element(i, value[i]);
            context.EndArray();
            return context.IsValid();
        }

        static bool Deserialize(SerializationContext& context, eastl::array<T, TSize>& value)
        {
            uint32_t size = 0;
            if (!context.BeginArray(size) || size != TSize)
                return false;

            for (uint32_t i = 0; i < size; ++i)
                context.Element(i, value[i]);
            context.EndArray();
            return context.IsValid();
        }

        static uint64_t GetSchemaHash()
        {
            return Internal::CombineSchemaHashes(Internal::CombineSchemaHashes(CompileTimeHash("array", 5), TSize),
                                                 Serialization::GetSchemaHash<T>());
        }

        static constexpr uint32_t GetVersion()
        {
            return 0;
        }
    };


    template<class T, class TAllocator>
    struct Serializer<eastl::vector<T, TAllocator>>
    {
        static bool Serialize(SerializationContext& context, const eastl::vector<T, TAllocator>& value)
        {
            uint32_t size = static_cast<uint32_t>(value.size());
            if (!context.BeginArray(size))
                return false;

            for (uint32_t i = 0; i < size; ++i)
                context.Element(i, value[i]);
            context.EndArray();
            return context.IsValid();
        }

        static bool Deserialize(SerializationContext& context, eastl::vector<T, TAllocator>& value)
        {
            uint32_t size = 0;
            if (!context.BeginArray(size))
                return false;

            value.resize(size);
            for (uint32_t i = 0; i < size; ++i)
                context.Element(i, value[i]);
            context.EndArray();
            return context.IsValid();
        }

        static uint64_t GetSchemaHash()
        {
            return Internal::CombineSchemaHashes(CompileTimeHash("vector", 6), Serialization::GetSchemaHash<T>());
        }

        static constexpr uint32_t GetVersion()
        {
            return 0;
        }
    };


    template<class TBase>
        requires requires(FE::Internal::StringImpl<TBase>& value, const char* data, const uint32_t size) {
            value.assign(data, size);
        }
    struct Serializer<FE::Internal::StringImpl<TBase>>
    {
        using StringType = FE::Internal::StringImpl<TBase>;

        static bool Serialize(SerializationContext& context, const StringType& value)
        {
            context.String(festd::string_view{ value.data(), value.size() });
            return context.IsValid();
        }

        static bool Deserialize(SerializationContext& context, StringType& value)
        {
            festd::string temporary;
            context.String(temporary);
            if (context.IsValid())
                value.assign(temporary.data(), temporary.size());
            return context.IsValid();
        }

        static constexpr uint64_t GetSchemaHash()
        {
            return CompileTimeHash("string", 6);
        }

        static constexpr uint32_t GetVersion()
        {
            return 0;
        }
    };


    template<>
    struct Serializer<Uuid>
    {
        static bool Serialize(SerializationContext& context, const Uuid& value);
        static bool Deserialize(SerializationContext& context, Uuid& value);

        static constexpr uint64_t GetSchemaHash()
        {
            return CompileTimeHash("uuid", 4);
        }

        static constexpr uint32_t GetVersion()
        {
            return 0;
        }
    };


    template<class T>
    void BindType(Rtti::Type& type)
    {
        if constexpr (Internal::HasSerializer<T>)
        {
            type.m_serialize = [](SerializationContext& context, const void* value) {
                return SerializeValue(context, *static_cast<const T*>(value));
            };
            type.m_deserialize = [](SerializationContext& context, void* value) {
                return DeserializeValue(context, *static_cast<T*>(value));
            };
            type.m_serializationVersion = GetVersion<T>();
            type.m_serializationSchemaHash = GetSchemaHash<T>();
        }
    }
} // namespace FE::Serialization
