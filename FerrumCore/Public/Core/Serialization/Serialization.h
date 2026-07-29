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


    struct SerializationContext;
    struct DeserializationContext;
    struct SerializationObject;
    struct DeserializationObject;
    struct SerializationArray;
    struct DeserializationArray;


    namespace Internal
    {
        template<class T>
        using ValueType = std::remove_cv_t<std::remove_reference_t<T>>;


        struct ContextState final
        {
            Direction m_direction = Direction::kSerialize;
            IO::IStream* m_stream = nullptr;
            bool m_isValid = true;
            uint32_t m_serializedVersion = 0;
            uint64_t m_serializedSchemaHash = 0;
            Error m_error;
        };
    } // namespace Internal


    template<class T>
    bool SerializeValue(SerializationContext& context, const T& value);


    template<class T>
    bool DeserializeValue(DeserializationContext& context, T& value);


    template<class T>
    uint64_t GetSchemaHash();


    template<class T>
    uint32_t GetVersion();


    struct SerializationFormat
    {
        virtual ~SerializationFormat() = default;

        SerializationFormat(const SerializationFormat&) = delete;
        SerializationFormat(SerializationFormat&&) = delete;
        SerializationFormat& operator=(const SerializationFormat&) = delete;
        SerializationFormat& operator=(SerializationFormat&&) = delete;

    protected:
        explicit SerializationFormat(const Format format)
            : m_format(format)
        {
        }

        [[nodiscard]] bool IsSerializing() const
        {
            return m_state->m_direction == Direction::kSerialize;
        }

        [[nodiscard]] bool IsDeserializing() const
        {
            return m_state->m_direction == Direction::kDeserialize;
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_state->m_isValid;
        }

        [[nodiscard]] IO::IStream* GetStream() const
        {
            return m_state->m_stream;
        }

        void SetSerializedVersion(const uint32_t version)
        {
            m_state->m_serializedVersion = version;
        }

        void SetSerializedSchemaHash(const uint64_t schemaHash)
        {
            m_state->m_serializedSchemaHash = schemaHash;
        }

        void Fail(const ErrorCode code, const uint64_t byteOffset = UINT64_MAX, const uint32_t line = 0,
                  const uint32_t column = 0)
        {
            if (!m_state->m_isValid)
                return;

            m_state->m_isValid = false;
            m_state->m_error.m_code = code;
            m_state->m_error.m_byteOffset = byteOffset == UINT64_MAX ? GetCurrentOffsetImpl() : byteOffset;
            m_state->m_error.m_line = line;
            m_state->m_error.m_column = column;
        }

    private:
        friend SerializationContext;
        friend DeserializationContext;
        friend SerializationObject;
        friend DeserializationObject;
        friend SerializationArray;
        friend DeserializationArray;

        void Begin(Internal::ContextState& state, IO::IStream* stream, const Direction direction)
        {
            state = {};
            state.m_direction = direction;
            state.m_stream = stream;
            m_state = &state;
            ResetImpl();
        }

        virtual void ResetImpl() = 0;
        virtual bool BeginDocumentImpl(Rtti::TypeID expectedType, uint32_t version, uint64_t schemaHash) = 0;
        virtual void EndDocumentImpl() = 0;
        virtual bool BeginObjectImpl() = 0;
        virtual void EndObjectImpl() = 0;
        virtual bool BeginFieldImpl(festd::ascii_view name, uint64_t fieldID) = 0;
        virtual void EndFieldImpl() = 0;
        virtual bool BeginArrayImpl(uint32_t& size) = 0;
        virtual void EndArrayImpl() = 0;
        virtual bool BeginElementImpl(uint32_t index) = 0;
        virtual void EndElementImpl() = 0;
        virtual void StoreScalarImpl(ScalarKind kind, const void* value, uint32_t byteSize) = 0;
        virtual void LoadScalarImpl(ScalarKind kind, void* value, uint32_t byteSize) = 0;
        virtual void StoreBytesImpl(const void* value, uint32_t byteSize) = 0;
        virtual void LoadBytesImpl(void* value, uint32_t byteSize) = 0;
        virtual void StoreStringImpl(festd::string_view value) = 0;
        virtual uint32_t LoadStringSizeImpl() = 0;
        virtual void LoadStringImpl(festd::span<char> buffer) = 0;
        [[nodiscard]] virtual uint64_t GetCurrentOffsetImpl() const = 0;

        const Format m_format;
        Internal::ContextState* m_state = nullptr;
    };


    struct SerializationContext final
    {
        SerializationContext(IO::IStream* stream, SerializationFormat& format)
            : m_stream(stream)
            , m_format(&format)
        {
            FE_Assert(stream != nullptr);
        }

        SerializationContext(const SerializationContext&) = delete;
        SerializationContext(SerializationContext&&) = delete;
        SerializationContext& operator=(const SerializationContext&) = delete;
        SerializationContext& operator=(SerializationContext&&) = delete;

        [[nodiscard]] Format GetFormat() const
        {
            return m_format->m_format;
        }

        [[nodiscard]] bool IsBinary() const
        {
            return GetFormat() != Format::kJson;
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_state.m_isValid;
        }

        [[nodiscard]] const Error& GetError() const
        {
            return m_state.m_error;
        }

        template<class T>
        bool Store(const T& value)
        {
            Begin();
            const Rtti::TypeID typeID = Rtti::GetTypeID<Internal::ValueType<T>>();
            if (!m_format->BeginDocumentImpl(typeID, GetVersion<T>(), GetSchemaHash<T>()))
                return false;

            if (!SerializeValue(*this, value))
                m_format->Fail(ErrorCode::kSerializerError);

            m_format->EndDocumentImpl();
            return IsValid();
        }

        bool Store(const Rtti::Type& type, const void* value);

        template<class T>
        SerializationContext& Number(const T& value)
        {
            static_assert(std::is_arithmetic_v<T>);
            m_format->StoreScalarImpl(GetScalarKind<T>(), &value, sizeof(T));
            return *this;
        }

        template<class T>
        SerializationContext& RawBytes(const T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            m_format->StoreBytesImpl(&value, sizeof(T));
            return *this;
        }

        SerializationContext& StoreString(const festd::string_view value)
        {
            m_format->StoreStringImpl(value);
            return *this;
        }

        SerializationObject BeginObject();
        SerializationArray BeginArray(uint32_t size);

        void ReportError(const ErrorCode code)
        {
            m_format->Fail(code);
        }

    private:
        friend SerializationObject;
        friend SerializationArray;

        IO::IStream* m_stream;
        SerializationFormat* m_format;
        Internal::ContextState m_state;

        void Begin()
        {
            m_format->Begin(m_state, m_stream, Direction::kSerialize);
        }

        template<class T>
        void Field(const festd::ascii_view name, const T& value)
        {
            if (!IsValid())
                return;
            if (!m_format->BeginFieldImpl(name, CompileTimeHash(name.data(), name.size())))
                return;

            if (!SerializeValue(*this, value))
                m_format->Fail(ErrorCode::kSerializerError);
            m_format->EndFieldImpl();
        }

        template<class T>
        void Element(const uint32_t index, const T& value)
        {
            if (!IsValid())
                return;
            if (!m_format->BeginElementImpl(index))
                return;

            if (!SerializeValue(*this, value))
                m_format->Fail(ErrorCode::kSerializerError);
            m_format->EndElementImpl();
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


    struct DeserializationContext final
    {
        DeserializationContext(IO::IStream* stream, SerializationFormat& format)
            : m_stream(stream)
            , m_format(&format)
        {
            FE_Assert(stream != nullptr);
        }

        DeserializationContext(const DeserializationContext&) = delete;
        DeserializationContext(DeserializationContext&&) = delete;
        DeserializationContext& operator=(const DeserializationContext&) = delete;
        DeserializationContext& operator=(DeserializationContext&&) = delete;

        [[nodiscard]] Format GetFormat() const
        {
            return m_format->m_format;
        }

        [[nodiscard]] bool IsBinary() const
        {
            return GetFormat() != Format::kJson;
        }

        [[nodiscard]] bool IsValid() const
        {
            return m_state.m_isValid;
        }

        [[nodiscard]] const Error& GetError() const
        {
            return m_state.m_error;
        }

        [[nodiscard]] uint32_t GetSerializedVersion() const
        {
            return m_state.m_serializedVersion;
        }

        [[nodiscard]] uint64_t GetSerializedSchemaHash() const
        {
            return m_state.m_serializedSchemaHash;
        }

        template<class T>
        bool Load(T& value)
        {
            Begin();
            const Rtti::TypeID typeID = Rtti::GetTypeID<Internal::ValueType<T>>();
            if (!m_format->BeginDocumentImpl(typeID, GetVersion<T>(), GetSchemaHash<T>()))
                return false;

            if (!DeserializeValue(*this, value))
                m_format->Fail(ErrorCode::kSerializerError);
            m_format->EndDocumentImpl();
            return IsValid();
        }

        bool Load(const Rtti::Type& type, void* value);

        template<class T>
        DeserializationContext& Number(T& value)
        {
            static_assert(std::is_arithmetic_v<T>);
            m_format->LoadScalarImpl(GetScalarKind<T>(), &value, sizeof(T));
            return *this;
        }

        template<class T>
        DeserializationContext& RawBytes(T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            m_format->LoadBytesImpl(&value, sizeof(T));
            return *this;
        }

        [[nodiscard]] uint32_t LoadStringSize()
        {
            return m_format->LoadStringSizeImpl();
        }

        DeserializationContext& LoadString(const festd::span<char> buffer)
        {
            m_format->LoadStringImpl(buffer);
            return *this;
        }

        DeserializationObject BeginObject();
        DeserializationArray BeginArray(uint32_t& size);

        void ReportError(const ErrorCode code)
        {
            m_format->Fail(code);
        }

    private:
        friend DeserializationObject;
        friend DeserializationArray;

        IO::IStream* m_stream;
        SerializationFormat* m_format;
        Internal::ContextState m_state;

        void Begin()
        {
            m_format->Begin(m_state, m_stream, Direction::kDeserialize);
        }

        template<class T>
        void Field(const festd::ascii_view name, T& value)
        {
            if (!IsValid())
                return;
            if (!m_format->BeginFieldImpl(name, CompileTimeHash(name.data(), name.size())))
                return;

            if (!DeserializeValue(*this, value))
                m_format->Fail(ErrorCode::kSerializerError);
            m_format->EndFieldImpl();
        }

        template<class T>
        void Element(const uint32_t index, T& value)
        {
            if (!IsValid())
                return;
            if (!m_format->BeginElementImpl(index))
                return;

            if (!DeserializeValue(*this, value))
                m_format->Fail(ErrorCode::kSerializerError);
            m_format->EndElementImpl();
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


    struct SerializationObject final
    {
        SerializationObject(const SerializationObject&) = delete;
        SerializationObject& operator=(const SerializationObject&) = delete;

        SerializationObject(SerializationObject&& other) noexcept
            : m_context(other.m_context)
        {
            other.m_context = nullptr;
        }

        ~SerializationObject()
        {
            if (m_context != nullptr)
                m_context->m_format->EndObjectImpl();
        }

        [[nodiscard]] explicit operator bool() const
        {
            return m_context != nullptr;
        }

        template<class T>
        SerializationObject& Field(const festd::ascii_view name, const T& value)
        {
            if (m_context != nullptr)
                m_context->Field(name, value);
            return *this;
        }

    private:
        friend SerializationContext;

        explicit SerializationObject(SerializationContext* context)
            : m_context(context)
        {
        }

        SerializationContext* m_context;
    };


    struct DeserializationObject final
    {
        DeserializationObject(const DeserializationObject&) = delete;
        DeserializationObject& operator=(const DeserializationObject&) = delete;

        DeserializationObject(DeserializationObject&& other) noexcept
            : m_context(other.m_context)
        {
            other.m_context = nullptr;
        }

        ~DeserializationObject()
        {
            if (m_context != nullptr)
                m_context->m_format->EndObjectImpl();
        }

        [[nodiscard]] explicit operator bool() const
        {
            return m_context != nullptr;
        }

        template<class T>
        DeserializationObject& Field(const festd::ascii_view name, T& value)
        {
            if (m_context != nullptr)
                m_context->Field(name, value);
            return *this;
        }

    private:
        friend DeserializationContext;

        explicit DeserializationObject(DeserializationContext* context)
            : m_context(context)
        {
        }

        DeserializationContext* m_context;
    };


    struct SerializationArray final
    {
        SerializationArray(const SerializationArray&) = delete;
        SerializationArray& operator=(const SerializationArray&) = delete;

        SerializationArray(SerializationArray&& other) noexcept
            : m_context(other.m_context)
        {
            other.m_context = nullptr;
        }

        ~SerializationArray()
        {
            if (m_context != nullptr)
                m_context->m_format->EndArrayImpl();
        }

        [[nodiscard]] explicit operator bool() const
        {
            return m_context != nullptr;
        }

        template<class T>
        SerializationArray& Element(const uint32_t index, const T& value)
        {
            if (m_context != nullptr)
                m_context->Element(index, value);
            return *this;
        }

    private:
        friend SerializationContext;

        explicit SerializationArray(SerializationContext* context)
            : m_context(context)
        {
        }

        SerializationContext* m_context;
    };


    struct DeserializationArray final
    {
        DeserializationArray(const DeserializationArray&) = delete;
        DeserializationArray& operator=(const DeserializationArray&) = delete;

        DeserializationArray(DeserializationArray&& other) noexcept
            : m_context(other.m_context)
        {
            other.m_context = nullptr;
        }

        ~DeserializationArray()
        {
            if (m_context != nullptr)
                m_context->m_format->EndArrayImpl();
        }

        [[nodiscard]] explicit operator bool() const
        {
            return m_context != nullptr;
        }

        template<class T>
        DeserializationArray& Element(const uint32_t index, T& value)
        {
            if (m_context != nullptr)
                m_context->Element(index, value);
            return *this;
        }

    private:
        friend DeserializationContext;

        explicit DeserializationArray(DeserializationContext* context)
            : m_context(context)
        {
        }

        DeserializationContext* m_context;
    };


    inline SerializationObject SerializationContext::BeginObject()
    {
        return SerializationObject{ IsValid() && m_format->BeginObjectImpl() ? this : nullptr };
    }


    inline SerializationArray SerializationContext::BeginArray(const uint32_t size)
    {
        uint32_t mutableSize = size;
        return SerializationArray{ IsValid() && m_format->BeginArrayImpl(mutableSize) ? this : nullptr };
    }


    inline DeserializationObject DeserializationContext::BeginObject()
    {
        return DeserializationObject{ IsValid() && m_format->BeginObjectImpl() ? this : nullptr };
    }


    inline DeserializationArray DeserializationContext::BeginArray(uint32_t& size)
    {
        return DeserializationArray{ IsValid() && m_format->BeginArrayImpl(size) ? this : nullptr };
    }


    namespace Internal
    {
        template<class T>
        concept HasSerializer = requires(SerializationContext& serializationContext,
                                         DeserializationContext& deserializationContext, const T& constValue, T& value) {
            { Serializer<T>::Serialize(serializationContext, constValue) } -> std::same_as<bool>;
            { Serializer<T>::Deserialize(deserializationContext, value) } -> std::same_as<bool>;
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
    bool DeserializeValue(DeserializationContext& context, T& value)
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

        static bool Deserialize(DeserializationContext& context, T& value)
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

        static bool Deserialize(DeserializationContext& context, T& value)
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


    template<class T, size_t TSize>
    struct Serializer<T[TSize]>
    {
        static bool Serialize(SerializationContext& context, const T (&value)[TSize])
        {
            if (auto array = context.BeginArray(static_cast<uint32_t>(TSize)))
            {
                for (uint32_t i = 0; i < TSize; ++i)
                    array.Element(i, value[i]);
                return context.IsValid();
            }
            return false;
        }

        static bool Deserialize(DeserializationContext& context, T (&value)[TSize])
        {
            uint32_t size = 0;
            if (auto array = context.BeginArray(size))
            {
                if (size != TSize)
                {
                    context.ReportError(ErrorCode::kMalformedData);
                    return false;
                }

                for (uint32_t i = 0; i < size; ++i)
                    array.Element(i, value[i]);
                return context.IsValid();
            }
            return false;
        }

        static constexpr uint64_t GetSchemaHash()
        {
            constexpr uint64_t kTypeNameHash = CompileTimeHash("c-array");
            constexpr uint64_t kSizeHash = HashCombine(kTypeNameHash, TSize);
            static uint64_t schemaHash = HashCombine(kSizeHash, Serialization::GetSchemaHash<T>());
            return schemaHash;
        }

        static constexpr uint32_t GetVersion()
        {
            return 0;
        }
    };


    template<class T, size_t TSize>
    struct Serializer<festd::array<T, TSize>>
    {
        static bool Serialize(SerializationContext& context, const festd::array<T, TSize>& value)
        {
            if (auto array = context.BeginArray(static_cast<uint32_t>(TSize)))
            {
                for (uint32_t i = 0; i < TSize; ++i)
                    array.Element(i, value[i]);
                return context.IsValid();
            }
            return false;
        }

        static bool Deserialize(DeserializationContext& context, festd::array<T, TSize>& value)
        {
            uint32_t size = 0;
            if (auto array = context.BeginArray(size))
            {
                if (size != TSize)
                {
                    context.ReportError(ErrorCode::kMalformedData);
                    return false;
                }

                for (uint32_t i = 0; i < size; ++i)
                    array.Element(i, value[i]);
                return context.IsValid();
            }
            return false;
        }

        static constexpr uint64_t GetSchemaHash()
        {
            constexpr uint64_t kTypeNameHash = CompileTimeHash("array");
            constexpr uint64_t kSizeHash = HashCombine(kTypeNameHash, TSize);
            static uint64_t schemaHash = HashCombine(kSizeHash, Serialization::GetSchemaHash<T>());
            return schemaHash;
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
            if (auto array = context.BeginArray(static_cast<uint32_t>(value.size())))
            {
                for (uint32_t i = 0; i < value.size(); ++i)
                    array.Element(i, value[i]);
                return context.IsValid();
            }
            return false;
        }

        static bool Deserialize(DeserializationContext& context, eastl::vector<T, TAllocator>& value)
        {
            uint32_t size = 0;
            if (auto array = context.BeginArray(size))
            {
                value.resize(size);
                for (uint32_t i = 0; i < size; ++i)
                    array.Element(i, value[i]);
                return context.IsValid();
            }
            return false;
        }

        static constexpr uint64_t GetSchemaHash()
        {
            constexpr uint64_t kTypeNameHash = CompileTimeHash("vector");
            static uint64_t schemaHash = HashCombine(kTypeNameHash, Serialization::GetSchemaHash<T>());
            return schemaHash;
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

        static bool Deserialize(DeserializationContext& context, T& value)
        {
            UnderlyingType underlyingValue{};
            if (!DeserializeValue(context, underlyingValue))
                return false;

            value = static_cast<T>(underlyingValue);
            return true;
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

        static bool Deserialize(DeserializationContext& context, StringType& value)
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
            constexpr uint64_t kTypeNameHash = CompileTimeHash("string");
            return kTypeNameHash;
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
        static bool Deserialize(DeserializationContext& context, Uuid& value);

        static constexpr uint64_t GetSchemaHash()
        {
            constexpr uint64_t kTypeNameHash = CompileTimeHash("Uuid");
            return kTypeNameHash;
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
            type.m_deserialize = [](DeserializationContext& context, void* value) {
                return DeserializeValue(context, *static_cast<T*>(value));
            };
            type.m_serializationVersion = GetVersion<T>();
            type.m_serializationSchemaHash = GetSchemaHash<T>();
        }
    }
} // namespace FE::Serialization
