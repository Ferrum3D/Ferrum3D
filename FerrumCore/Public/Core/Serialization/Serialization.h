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


    enum class ResultCode : int32_t
    {
        kSuccess = 0,
        kSerializerError = -1,
        kStreamReadFailed = -2,
        kStreamWriteFailed = -3,
        kInvalidHeader = -4,
        kTypeMismatch = -5,
        kSchemaMismatch = -6,
        kMalformedData = -7,
        kSizeLimitExceeded = -8,
        kJsonParseError = -9,
        kInvalidNumber = -10,
        kInvalidString = -11,
        kUnsupportedValue = -12,
        kUnknownError = kDefaultErrorCode<ResultCode>,
    };


    struct Error final
    {
        ResultCode m_code = ResultCode::kSuccess;
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
    } // namespace Internal


    template<class T>
    ResultCode SerializeValue(SerializationContext& context, const T& value);


    template<class T>
    ResultCode DeserializeValue(DeserializationContext& context, T& value);


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

        [[nodiscard]] bool IsValid() const
        {
            return m_error.m_code == ResultCode::kSuccess;
        }

        ResultCode Fail(ResultCode code, uint64_t byteOffset = Constants::kMaxU64, uint32_t line = 0, uint32_t column = 0);

        bool m_isDocumentActive = false;
        Format m_format;
        Direction m_direction = Direction::kSerialize;
        uint32_t m_serializedVersion = 0;
        uint64_t m_serializedSchemaHash = 0;
        Error m_error;
        IO::IStream* m_stream = nullptr;

    private:
        friend SerializationContext;
        friend DeserializationContext;
        friend SerializationObject;
        friend DeserializationObject;
        friend SerializationArray;
        friend DeserializationArray;

        ResultCode BeginDocument(IO::IStream* stream, Direction direction, Rtti::TypeID expectedType, uint32_t version,
                                 uint64_t schemaHash);

        ResultCode EndDocument();

        ResultCode Record(const ResultCode code)
        {
            return Fail(code);
        }

        virtual void ResetImpl() = 0;
        virtual ResultCode BeginStoreDocumentImpl(Rtti::TypeID expectedType, uint32_t version, uint64_t schemaHash) = 0;
        virtual ResultCode BeginLoadDocumentImpl(Rtti::TypeID expectedType, uint32_t version, uint64_t schemaHash) = 0;
        virtual ResultCode EndStoreDocumentImpl() = 0;
        virtual ResultCode EndLoadDocumentImpl() = 0;
        virtual ResultCode BeginStoreObjectImpl() = 0;
        virtual ResultCode BeginLoadObjectImpl() = 0;
        virtual ResultCode EndStoreObjectImpl() = 0;
        virtual ResultCode EndLoadObjectImpl() = 0;
        virtual ResultCode BeginStoreFieldImpl(festd::ascii_view name, uint64_t fieldID) = 0;
        virtual ResultCode BeginLoadFieldImpl(festd::ascii_view name, uint64_t fieldID, bool& exists) = 0;
        virtual ResultCode EndStoreFieldImpl() = 0;
        virtual ResultCode EndLoadFieldImpl() = 0;
        virtual ResultCode BeginStoreArrayImpl(uint32_t size) = 0;
        virtual ResultCode BeginLoadArrayImpl(uint32_t& size) = 0;
        virtual ResultCode EndStoreArrayImpl() = 0;
        virtual ResultCode EndLoadArrayImpl() = 0;
        virtual ResultCode BeginStoreElementImpl(uint32_t index) = 0;
        virtual ResultCode BeginLoadElementImpl(uint32_t index) = 0;
        virtual ResultCode EndStoreElementImpl() = 0;
        virtual ResultCode EndLoadElementImpl() = 0;
        virtual ResultCode StoreScalarImpl(ScalarKind kind, const void* value, uint32_t byteSize) = 0;
        virtual ResultCode LoadScalarImpl(ScalarKind kind, void* value, uint32_t byteSize) = 0;
        virtual ResultCode StoreBytesImpl(const void* value, uint32_t byteSize) = 0;
        virtual ResultCode LoadBytesImpl(void* value, uint32_t byteSize) = 0;
        virtual ResultCode StoreStringImpl(festd::string_view value) = 0;
        virtual ResultCode LoadStringSizeImpl(uint32_t& size) = 0;
        virtual ResultCode LoadStringImpl(festd::span<char> buffer) = 0;
        [[nodiscard]] virtual uint64_t GetStoreCurrentOffsetImpl() const = 0;
        [[nodiscard]] virtual uint64_t GetLoadCurrentOffsetImpl() const = 0;
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
            return m_format->IsValid();
        }

        [[nodiscard]] const Error& GetError() const
        {
            return m_format->m_error;
        }

        [[nodiscard]] ResultCode GetResultCode() const
        {
            return m_format->m_error.m_code;
        }

        template<class T>
        ResultCode Store(const T& value)
        {
            const Rtti::TypeID typeID = Rtti::GetTypeID<Internal::ValueType<T>>();
            ResultCode result =
                m_format->BeginDocument(m_stream, Direction::kSerialize, typeID, GetVersion<T>(), GetSchemaHash<T>());
            if (result != ResultCode::kSuccess)
                return result;

            m_format->Record(SerializeValue(*this, value));
            return m_format->EndDocument();
        }

        ResultCode Store(const Rtti::Type& type, const void* value);

        template<class T>
            requires std::is_arithmetic_v<T>
        ResultCode Number(const T& value)
        {
            return m_format->Record(m_format->StoreScalarImpl(GetScalarKind<T>(), &value, sizeof(T)));
        }

        template<class T>
            requires std::is_trivially_copyable_v<T>
        ResultCode RawBytes(const T& value)
        {
            return m_format->Record(m_format->StoreBytesImpl(&value, sizeof(T)));
        }

        ResultCode StoreString(const festd::string_view value)
        {
            return m_format->Record(m_format->StoreStringImpl(value));
        }

        SerializationObject BeginObject();
        SerializationArray BeginArray(uint32_t size);

        ResultCode ReportError(const ResultCode code)
        {
            return m_format->Record(code);
        }

    private:
        friend SerializationObject;
        friend SerializationArray;

        IO::IStream* m_stream;
        SerializationFormat* m_format;

        template<class T>
        ResultCode Field(const festd::ascii_view name, const T& value)
        {
            if (!IsValid())
                return GetResultCode();

            const ResultCode result = m_format->BeginStoreFieldImpl(name, DefaultHash(name));
            m_format->Record(result);
            if (result != ResultCode::kSuccess)
                return GetResultCode();

            m_format->Record(SerializeValue(*this, value));
            m_format->Record(m_format->EndStoreFieldImpl());
            return GetResultCode();
        }

        template<class T>
        ResultCode Element(const uint32_t index, const T& value)
        {
            if (!IsValid())
                return GetResultCode();

            const ResultCode result = m_format->BeginStoreElementImpl(index);
            m_format->Record(result);
            if (result != ResultCode::kSuccess)
                return result;

            m_format->Record(SerializeValue(*this, value));
            m_format->Record(m_format->EndStoreElementImpl());
            return GetResultCode();
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
            return m_format->IsValid();
        }

        [[nodiscard]] const Error& GetError() const
        {
            return m_format->m_error;
        }

        [[nodiscard]] ResultCode GetResultCode() const
        {
            return m_format->m_error.m_code;
        }

        [[nodiscard]] uint32_t GetSerializedVersion() const
        {
            return m_format->m_serializedVersion;
        }

        [[nodiscard]] uint64_t GetSerializedSchemaHash() const
        {
            return m_format->m_serializedSchemaHash;
        }

        template<class T>
        ResultCode Load(T& value)
        {
            const Rtti::TypeID typeID = Rtti::GetTypeID<Internal::ValueType<T>>();
            ResultCode result =
                m_format->BeginDocument(m_stream, Direction::kDeserialize, typeID, GetVersion<T>(), GetSchemaHash<T>());
            if (result != ResultCode::kSuccess)
                return result;

            m_format->Record(DeserializeValue(*this, value));
            return m_format->EndDocument();
        }

        ResultCode Load(const Rtti::Type& type, void* value);

        template<class T>
        ResultCode Number(T& value)
        {
            static_assert(std::is_arithmetic_v<T>);
            return m_format->Record(m_format->LoadScalarImpl(GetScalarKind<T>(), &value, sizeof(T)));
        }

        template<class T>
        ResultCode RawBytes(T& value)
        {
            static_assert(std::is_trivially_copyable_v<T>);
            return m_format->Record(m_format->LoadBytesImpl(&value, sizeof(T)));
        }

        ResultCode LoadStringSize(uint32_t& size)
        {
            return m_format->Record(m_format->LoadStringSizeImpl(size));
        }

        ResultCode LoadString(const festd::span<char> buffer)
        {
            return m_format->Record(m_format->LoadStringImpl(buffer));
        }

        DeserializationObject BeginObject();
        DeserializationArray BeginArray(uint32_t& size);

        ResultCode ReportError(const ResultCode code)
        {
            return m_format->Record(code);
        }

    private:
        friend DeserializationObject;
        friend DeserializationArray;

        IO::IStream* m_stream;
        SerializationFormat* m_format;

        template<class T>
        ResultCode Field(const festd::ascii_view name, T& value)
        {
            if (!IsValid())
                return GetResultCode();

            bool exists = false;
            const ResultCode result = m_format->BeginLoadFieldImpl(name, DefaultHash(name), exists);
            m_format->Record(result);
            if (result != ResultCode::kSuccess || !exists)
                return GetResultCode();

            m_format->Record(DeserializeValue(*this, value));
            m_format->Record(m_format->EndLoadFieldImpl());
            return GetResultCode();
        }

        template<class T>
        ResultCode Element(const uint32_t index, T& value)
        {
            if (!IsValid())
                return GetResultCode();

            const ResultCode result = m_format->BeginLoadElementImpl(index);
            m_format->Record(result);
            if (result != ResultCode::kSuccess)
                return result;

            m_format->Record(DeserializeValue(*this, value));
            m_format->Record(m_format->EndLoadElementImpl());
            return GetResultCode();
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
                m_context->m_format->Record(m_context->m_format->EndStoreObjectImpl());
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
                m_context->m_format->Record(m_context->m_format->EndLoadObjectImpl());
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
                m_context->m_format->Record(m_context->m_format->EndStoreArrayImpl());
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
                m_context->m_format->Record(m_context->m_format->EndLoadArrayImpl());
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


    namespace Internal
    {
        template<class T>
        concept HasMemberSerializer = requires(SerializationContext& serializationContext,
                                               DeserializationContext& deserializationContext, const T& constValue, T& value) {
            { constValue.Serialize(serializationContext) } -> std::same_as<ResultCode>;
            { value.Deserialize(deserializationContext) } -> std::same_as<ResultCode>;
        };


        template<class T>
        concept HasStaticSerializer = requires(SerializationContext& serializationContext,
                                               DeserializationContext& deserializationContext, const T& constValue, T& value) {
            { T::Serialize(serializationContext, constValue) } -> std::same_as<ResultCode>;
            { T::Deserialize(deserializationContext, value) } -> std::same_as<ResultCode>;
        };


        template<class T>
        concept HasSerializer = requires(SerializationContext& serializationContext,
                                         DeserializationContext& deserializationContext, const T& constValue, T& value) {
            { Serializer<T>::Serialize(serializationContext, constValue) } -> std::same_as<ResultCode>;
            { Serializer<T>::Deserialize(deserializationContext, value) } -> std::same_as<ResultCode>;
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
    ResultCode SerializeValue(SerializationContext& context, const T& value)
    {
        using ValueType = Internal::ValueType<T>;
        static_assert(Internal::HasSerializer<ValueType>, "No serializer is defined for this type");
        return Serializer<ValueType>::Serialize(context, value);
    }


    template<class T>
    ResultCode DeserializeValue(DeserializationContext& context, T& value)
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
        else if constexpr (requires { ValueType::kVersion; })
            return static_cast<uint32_t>(ValueType::kVersion);
        else
            return 0;
    }


    template<class T>
        requires(Internal::HasMemberSerializer<T> || Internal::HasStaticSerializer<T>)
    struct Serializer<T>
    {
        static ResultCode Serialize(SerializationContext& context, const T& value)
        {
            if constexpr (Internal::HasMemberSerializer<T>)
                return value.Serialize(context);
            else
                return T::Serialize(context, value);
        }

        static ResultCode Deserialize(DeserializationContext& context, T& value)
        {
            if constexpr (Internal::HasMemberSerializer<T>)
                return value.Deserialize(context);
            else
                return T::Deserialize(context, value);
        }

        static uint64_t GetSchemaHash()
        {
            if constexpr (requires { T::RTTI_GetSerializationSchemaHash(); })
                return T::RTTI_GetSerializationSchemaHash();
            else
                return TypeNameHash<T>;
        }

        static uint32_t GetVersion()
        {
            if constexpr (requires { T::RTTI_GetSerializationVersion(); })
                return T::RTTI_GetSerializationVersion();
            else if constexpr (requires { T::kVersion; })
                return static_cast<uint32_t>(T::kVersion);
            else
                return 0;
        }
    };


    template<class T>
        requires std::is_arithmetic_v<T>
    struct Serializer<T>
    {
        static ResultCode Serialize(SerializationContext& context, const T& value)
        {
            return context.Number(value);
        }

        static ResultCode Deserialize(DeserializationContext& context, T& value)
        {
            return context.Number(value);
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
        static ResultCode Serialize(SerializationContext& context, const T (&value)[TSize])
        {
            if (auto array = context.BeginArray(static_cast<uint32_t>(TSize)))
            {
                for (uint32_t i = 0; i < TSize; ++i)
                    array.Element(i, value[i]);

                return context.GetResultCode();
            }

            return context.GetResultCode();
        }

        static ResultCode Deserialize(DeserializationContext& context, T (&value)[TSize])
        {
            uint32_t size = 0;
            if (auto array = context.BeginArray(size))
            {
                if (size != TSize)
                    return context.ReportError(ResultCode::kMalformedData);

                for (uint32_t i = 0; i < size; ++i)
                    array.Element(i, value[i]);

                return context.GetResultCode();
            }

            return context.GetResultCode();
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
        static ResultCode Serialize(SerializationContext& context, const festd::array<T, TSize>& value)
        {
            if (auto array = context.BeginArray(static_cast<uint32_t>(TSize)))
            {
                for (uint32_t i = 0; i < TSize; ++i)
                    array.Element(i, value[i]);

                return context.GetResultCode();
            }

            return context.GetResultCode();
        }

        static ResultCode Deserialize(DeserializationContext& context, festd::array<T, TSize>& value)
        {
            uint32_t size = 0;
            if (auto array = context.BeginArray(size))
            {
                if (size != TSize)
                    return context.ReportError(ResultCode::kMalformedData);

                for (uint32_t i = 0; i < size; ++i)
                    array.Element(i, value[i]);

                return context.GetResultCode();
            }

            return context.GetResultCode();
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
        static ResultCode Serialize(SerializationContext& context, const eastl::vector<T, TAllocator>& value)
        {
            if (auto array = context.BeginArray(static_cast<uint32_t>(value.size())))
            {
                for (uint32_t i = 0; i < value.size(); ++i)
                    array.Element(i, value[i]);

                return context.GetResultCode();
            }

            return context.GetResultCode();
        }

        static ResultCode Deserialize(DeserializationContext& context, eastl::vector<T, TAllocator>& value)
        {
            uint32_t size = 0;
            if (auto array = context.BeginArray(size))
            {
                value.resize(size);
                for (uint32_t i = 0; i < size; ++i)
                    array.Element(i, value[i]);

                return context.GetResultCode();
            }

            return context.GetResultCode();
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

        static ResultCode Serialize(SerializationContext& context, const T value)
        {
            const UnderlyingType underlyingValue = static_cast<UnderlyingType>(value);
            return SerializeValue(context, underlyingValue);
        }

        static ResultCode Deserialize(DeserializationContext& context, T& value)
        {
            UnderlyingType underlyingValue{};
            const ResultCode result = DeserializeValue(context, underlyingValue);
            if (result != ResultCode::kSuccess)
                return result;

            value = static_cast<T>(underlyingValue);
            return ResultCode::kSuccess;
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
            value.resize_uninitialized(size);
        }
    struct Serializer<FE::Internal::StringImpl<TBase>>
    {
        using StringType = FE::Internal::StringImpl<TBase>;

        static ResultCode Serialize(SerializationContext& context, const StringType& value)
        {
            return context.StoreString(festd::string_view{ value.data(), value.size() });
        }

        static ResultCode Deserialize(DeserializationContext& context, StringType& value)
        {
            uint32_t size = 0;
            const ResultCode result = context.LoadStringSize(size);
            if (result != ResultCode::kSuccess)
                return result;

            value.resize_uninitialized(size);
            return context.LoadString(festd::span<char>{ value.data(), size });
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
        static ResultCode Serialize(SerializationContext& context, const Uuid& value);
        static ResultCode Deserialize(DeserializationContext& context, Uuid& value);

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
} // namespace FE::Serialization
