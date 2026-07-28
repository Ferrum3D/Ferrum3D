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


    enum class ErrorCode : uint8_t
    {
        kNone,
        kSerializerError,
        kStreamReadFailed,
        kStreamWriteFailed,
        kInvalidHeader,
        kTypeMismatch,
        kSchemaMismatch,
        kMalformedData,
        kSizeLimitExceeded,
        kJsonParseError,
        kInvalidNumber,
        kInvalidString,
        kUnsupportedValue,
    };


    struct Error final
    {
        ErrorCode m_code = ErrorCode::kNone;
        uint64_t m_byteOffset = 0;
        uint32_t m_line = 0;
        uint32_t m_column = 0;
    };


    template<class T, class = void>
    struct Serializer;


    class SerializationContext;
    class ObjectScope;


    namespace Internal
    {
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

        [[nodiscard]] const Error& GetError() const
        {
            return m_error;
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
                Fail(ErrorCode::kSerializerError);
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
                Fail(ErrorCode::kSerializerError);
            EndDocument();
            return m_isValid;
        }

        bool Store(const Rtti::Type& type, const void* value);
        bool Load(const Rtti::Type& type, void* value);

        template<class T>
        SerializationContext& Element(const uint32_t index, const T& value)
        {
            if (!m_isValid || !BeginElement(index))
                return *this;

            if (!SerializeValue(*this, value))
                Fail(ErrorCode::kSerializerError);
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
                    Fail(ErrorCode::kSerializerError);
            }
            else
            {
                if (!DeserializeValue(*this, value))
                    Fail(ErrorCode::kSerializerError);
            }

            EndElement();
            return *this;
        }

        template<class T>
        SerializationContext& Number(T& value)
        {
            static_assert(std::is_arithmetic_v<T>);
            if (IsSerializing())
                StoreScalarImpl(GetScalarKind<T>(), &value, sizeof(T));
            else
                LoadScalarImpl(GetScalarKind<T>(), &value, sizeof(T));
            return *this;
        }

        template<class T>
        SerializationContext& Number(const T& value)
        {
            static_assert(std::is_arithmetic_v<T>);
            FE_Assert(IsSerializing());
            StoreScalarImpl(GetScalarKind<T>(), &value, sizeof(T));
            return *this;
        }

        template<class T>
        SerializationContext& RawBytes(T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            if (IsSerializing())
                StoreBytesImpl(&value, sizeof(T));
            else
                LoadBytesImpl(&value, sizeof(T));
            return *this;
        }

        template<class T>
        SerializationContext& RawBytes(const T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            FE_Assert(IsSerializing());
            StoreBytesImpl(&value, sizeof(T));
            return *this;
        }

        SerializationContext& StoreString(const festd::string_view value)
        {
            FE_Assert(IsSerializing());
            StoreStringImpl(value);
            return *this;
        }

        [[nodiscard]] uint32_t LoadStringSize()
        {
            FE_Assert(IsDeserializing());
            return LoadStringSizeImpl();
        }

        SerializationContext& LoadString(const festd::span<char> buffer)
        {
            FE_Assert(IsDeserializing());
            LoadStringImpl(buffer);
            return *this;
        }

        ObjectScope BeginObject();

        void ReportError(const ErrorCode code)
        {
            Fail(code);
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

        void Fail(const ErrorCode code, const uint64_t byteOffset = UINT64_MAX, const uint32_t line = 0,
                  const uint32_t column = 0)
        {
            if (!m_isValid)
                return;

            m_isValid = false;
            m_error.m_code = code;
            m_error.m_byteOffset = byteOffset == UINT64_MAX ? GetCurrentOffset() : byteOffset;
            m_error.m_line = line;
            m_error.m_column = column;
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
        virtual void StoreScalarImpl(ScalarKind kind, const void* value, uint32_t byteSize) = 0;
        virtual void LoadScalarImpl(ScalarKind kind, void* value, uint32_t byteSize) = 0;
        virtual void StoreBytesImpl(const void* value, uint32_t byteSize) = 0;
        virtual void LoadBytesImpl(void* value, uint32_t byteSize) = 0;
        virtual void StoreStringImpl(festd::string_view value) = 0;
        virtual uint32_t LoadStringSizeImpl() = 0;
        virtual void LoadStringImpl(festd::span<char> buffer) = 0;
        [[nodiscard]] virtual uint64_t GetCurrentOffset() const = 0;

        Direction m_direction = Direction::kSerialize;
        const Format m_format;
        bool m_isValid = true;
        uint32_t m_serializedVersion = 0;
        uint64_t m_serializedSchemaHash = 0;
        Error m_error;

    private:
        void Begin(const Direction direction)
        {
            m_direction = direction;
            m_isValid = true;
            m_serializedVersion = 0;
            m_serializedSchemaHash = 0;
            m_error = {};
            Reset();
        }

        template<class T>
        void Field(const festd::ascii_view name, const T& value)
        {
            if (!m_isValid || !BeginField(name, CompileTimeHash(name.data(), name.size())))
                return;

            if (!SerializeValue(*this, value))
                Fail(ErrorCode::kSerializerError);
            EndField();
        }

        template<class T>
        void Field(const festd::ascii_view name, T& value)
        {
            if (!m_isValid || !BeginField(name, CompileTimeHash(name.data(), name.size())))
                return;

            if (IsSerializing())
            {
                if (!SerializeValue(*this, value))
                    Fail(ErrorCode::kSerializerError);
            }
            else if (!DeserializeValue(*this, value))
            {
                Fail(ErrorCode::kSerializerError);
            }

            EndField();
        }

        friend ObjectScope;

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


    class ObjectScope final
    {
    public:
        ObjectScope(const ObjectScope&) = delete;
        ObjectScope& operator=(const ObjectScope&) = delete;

        ObjectScope(ObjectScope&& other) noexcept
            : m_context(other.m_context)
        {
            other.m_context = nullptr;
        }

        ~ObjectScope()
        {
            if (m_context != nullptr)
                m_context->EndObjectImpl();
        }

        [[nodiscard]] explicit operator bool() const
        {
            return m_context != nullptr;
        }

        template<class T>
        ObjectScope& Field(const festd::ascii_view name, const T& value)
        {
            if (m_context != nullptr)
                m_context->Field(name, value);
            return *this;
        }

        template<class T>
        ObjectScope& Field(const festd::ascii_view name, T& value)
        {
            if (m_context != nullptr)
                m_context->Field(name, value);
            return *this;
        }

    private:
        friend SerializationContext;

        explicit ObjectScope(SerializationContext* context)
            : m_context(context)
        {
        }

        SerializationContext* m_context;
    };


    inline ObjectScope SerializationContext::BeginObject()
    {
        return ObjectScope{ m_isValid && BeginObjectImpl() ? this : nullptr };
    }


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

        static uint64_t GetSchemaHash()
        {
            static const uint64_t kHash = [] {
                Hasher hasher;
                hasher.Update("enum", 4);
                hasher.UpdateRaw(TypeNameHash<T>);
                return hasher.Finalize();
            }();
            return kHash;
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
            if (!context.BeginArray(size))
                return false;
            if (size != TSize)
            {
                context.ReportError(ErrorCode::kMalformedData);
                return false;
            }

            for (uint32_t i = 0; i < size; ++i)
                context.Element(i, value[i]);
            context.EndArray();
            return context.IsValid();
        }

        static uint64_t GetSchemaHash()
        {
            static const uint64_t kHash = [] {
                Hasher hasher;
                hasher.Update("array", 5);
                hasher.Update(TSize);
                hasher.UpdateRaw(Serialization::GetSchemaHash<T>());
                return hasher.Finalize();
            }();
            return kHash;
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
            if (!context.BeginArray(size))
                return false;
            if (size != TSize)
            {
                context.ReportError(ErrorCode::kMalformedData);
                return false;
            }

            for (uint32_t i = 0; i < size; ++i)
                context.Element(i, value[i]);
            context.EndArray();
            return context.IsValid();
        }

        static uint64_t GetSchemaHash()
        {
            static const uint64_t kHash = [] {
                Hasher hasher;
                hasher.Update("array", 5);
                hasher.Update(TSize);
                hasher.UpdateRaw(Serialization::GetSchemaHash<T>());
                return hasher.Finalize();
            }();
            return kHash;
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
            static const uint64_t kHash = [] {
                Hasher hasher;
                hasher.Update("vector", 6);
                hasher.UpdateRaw(Serialization::GetSchemaHash<T>());
                return hasher.Finalize();
            }();
            return kHash;
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
            context.StoreString(festd::string_view{ value.data(), value.size() });
            return context.IsValid();
        }

        static bool Deserialize(SerializationContext& context, StringType& value)
        {
            const uint32_t size = context.LoadStringSize();
            if (!context.IsValid())
                return false;

            value.resize_uninitialized(size);
            context.LoadString(festd::span<char>{ value.data(), size });
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
