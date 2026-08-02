#include <Core/IO/StreamBase.h>
#include <Core/Math/Aabb.h>
#include <Core/Math/Color.h>
#include <Core/Math/Matrix4x4.h>
#include <Core/Math/Obb.h>
#include <Core/Math/Rect.h>
#include <Core/Math/Sphere.h>
#include <Core/Math/Transform.h>
#include <Core/Math/Vector3Int.h>
#include <Core/Math/Vector3UInt.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Serialization/JsonSerialization.h>
#include <Serialization/SerializationTypes.h>
#include <gtest/gtest.h>

namespace FE::Serialization::Tests
{
    namespace
    {
        struct MemoryStream final : public IO::BufferedStream
        {
            MemoryStream()
                : BufferedStream(nullptr)
            {
            }

            ~MemoryStream() override
            {
                FlushWrites();
            }

            [[nodiscard]] bool SeekAllowed() const override
            {
                return true;
            }

            [[nodiscard]] bool IsOpen() const override
            {
                return true;
            }

            IO::ResultCode Seek(const intptr_t offset, const IO::SeekMode seekMode) override
            {
                FlushWrites();
                intptr_t base = 0;
                if (seekMode == IO::SeekMode::kCurrent)
                    base = static_cast<intptr_t>(m_position);
                else if (seekMode == IO::SeekMode::kEnd)
                    base = static_cast<intptr_t>(m_data.size());

                const intptr_t position = base + offset;
                if (position < 0 || static_cast<size_t>(position) > m_data.size())
                    return IO::ResultCode::kInvalidSeek;

                m_position = static_cast<size_t>(position);
                return IO::ResultCode::kSuccess;
            }

            [[nodiscard]] uintptr_t Tell() const override
            {
                return m_position;
            }

            [[nodiscard]] size_t Length() const override
            {
                return m_data.size();
            }

            size_t ReadToBuffer(void* buffer, const size_t byteSize) override
            {
                FlushWrites();
                const size_t bytesRead = std::min(byteSize, m_data.size() - m_position);
                memcpy(buffer, m_data.data() + m_position, bytesRead);
                m_position += bytesRead;
                return bytesRead;
            }

            festd::string_view GetName() override
            {
                return "MemoryStream";
            }

            [[nodiscard]] IO::OpenMode GetOpenMode() const override
            {
                return IO::OpenMode::kReadWrite;
            }

            void Close() override {}

            void Rewind()
            {
                ASSERT_EQ(Seek(0, IO::SeekMode::kBegin), IO::ResultCode::kSuccess);
            }

            [[nodiscard]] festd::span<const std::byte> GetData()
            {
                FlushWrites();
                return m_data;
            }

            [[nodiscard]] festd::span<std::byte> GetMutableData()
            {
                FlushWrites();
                return m_data;
            }

        private:
            festd::vector<std::byte> m_data;
            size_t m_position = 0;

            void DoRelease() override
            {
                FE_Assert(false, "Stack-owned test stream cannot be released");
            }

            size_t WriteImpl(const void* buffer, const size_t byteSize) override
            {
                if (m_position + byteSize > m_data.size())
                    m_data.resize(static_cast<uint32_t>(m_position + byteSize));

                memcpy(m_data.data() + m_position, buffer, byteSize);
                m_position += byteSize;
                return byteSize;
            }
        };


        struct ManualObject final
        {
            static constexpr uint32_t kVersion = 7;

            uint32_t m_count = 0;
            festd::string m_name;

            static ResultCode Serialize(SerializationContext& context, const ManualObject& value)
            {
                if (auto object = context.BeginObject())
                {
                    object.Field("m_count", value.m_count).Field("m_name", value.m_name);
                    return context.GetResultCode();
                }

                return context.GetResultCode();
            }

            static ResultCode Deserialize(DeserializationContext& context, ManualObject& value)
            {
                if (auto object = context.BeginObject())
                {
                    object.Field("m_count", value.m_count).Field("m_name", value.m_name);
                    return context.GetResultCode();
                }

                return context.GetResultCode();
            }
        };


        TestObject CreateObject()
        {
            TestObject result;
            result.m_id = Uuid::Parse("5D1CFA6C-47A0-4C05-9F40-7220B76ABEC1");
            result.m_desc.m_width = 1023;
            result.m_desc.m_height = 777;
            result.m_desc.m_flags = 4095;
            result.m_desc.m_scale = 0.123f;
            result.m_values = { 1, 2, 3, 5, 8 };
            result.m_coordinates = { 1.25f, -2.5f, 4.0f };
            result.m_name = "serialization";
            result.m_transient = 42;
            return result;
        }


        void ExpectEqual(const TestObject& lhs, const TestObject& rhs)
        {
            EXPECT_EQ(lhs.m_id, rhs.m_id);
            EXPECT_EQ(lhs.m_desc.m_width, rhs.m_desc.m_width);
            EXPECT_EQ(lhs.m_desc.m_height, rhs.m_desc.m_height);
            EXPECT_EQ(lhs.m_desc.m_flags, rhs.m_desc.m_flags);
            EXPECT_EQ(lhs.m_desc.m_scale, rhs.m_desc.m_scale);
            EXPECT_EQ(lhs.m_values, rhs.m_values);
            EXPECT_EQ(lhs.m_coordinates, rhs.m_coordinates);
            EXPECT_EQ(lhs.m_name, rhs.m_name);
        }


        template<class TFormat>
        void TestRoundTrip()
        {
            MemoryStream stream;
            TestObject source = CreateObject();
            TFormat format;
            SerializationContext writer(&stream, format);
            ASSERT_EQ(writer.Store(source), ResultCode::kSuccess);

            stream.Rewind();
            TestObject destination;
            destination.m_transient = 99;
            DeserializationContext reader(&stream, format);
            ASSERT_EQ(reader.Load(destination), ResultCode::kSuccess);
            ExpectEqual(source, destination);
            EXPECT_EQ(destination.m_transient, 99);
        }


        template<class T>
            requires requires(const T& value) { value.m_values; }
        void ExpectMathValueEqual(const T& expected, const T& actual)
        {
            constexpr size_t kValueCount = sizeof(expected.m_values) / sizeof(expected.m_values[0]);
            for (size_t i = 0; i < kValueCount; ++i)
                EXPECT_EQ(actual.m_values[i], expected.m_values[i]);
        }


        void ExpectMathValueEqual(const Sphere& expected, const Sphere& actual)
        {
            ExpectMathValueEqual(expected.m_centerRadius, actual.m_centerRadius);
        }


        void ExpectMathValueEqual(const Obb& expected, const Obb& actual)
        {
            ExpectMathValueEqual(expected.center, actual.center);
            ExpectMathValueEqual(expected.extents, actual.extents);
            ExpectMathValueEqual(expected.rotation, actual.rotation);
        }


        void ExpectMathValueEqual(const Transform& expected, const Transform& actual)
        {
            ExpectMathValueEqual(expected.m_translationScale, actual.m_translationScale);
            ExpectMathValueEqual(expected.m_rotation, actual.m_rotation);
        }


        template<class TFormat, class T>
        void TestMathValueRoundTrip(const T& source)
        {
            const Rtti::Type& type = Rtti::GetType<T>();
            ASSERT_NE(type.m_serialize, nullptr);
            ASSERT_NE(type.m_deserialize, nullptr);
            if constexpr (requires { T::RTTI_GetSerializationSchemaHash(); })
                ASSERT_NE(type.m_serializationSchemaHash, 0);
            else
                ASSERT_EQ(type.m_serializationSchemaHash, TypeNameHash<T>);

            MemoryStream stream;
            TFormat format;
            SerializationContext writer(&stream, format);
            ASSERT_EQ(writer.Store(source), ResultCode::kSuccess);

            [[maybe_unused]] festd::string_view text;
            if constexpr (std::is_same_v<TFormat, JsonFormat>)
            {
                text = festd::string_view{ reinterpret_cast<const char*>(stream.GetData().data()), stream.GetData().size() };
                ASSERT_GT(text.size(), 0);
            }

            stream.Rewind();
            T destination{};
            DeserializationContext reader(&stream, format);
            ASSERT_EQ(reader.Load(destination), ResultCode::kSuccess);
            ExpectMathValueEqual(source, destination);
        }


        template<class TFormat>
        void TestMathValuesRoundTrip()
        {
            TestMathValueRoundTrip<TFormat>(Vector2{ 1.25f, -2.5f });
            TestMathValueRoundTrip<TFormat>(Vector2Int{ -12, 34 });
            TestMathValueRoundTrip<TFormat>(Vector2UInt{ 56, 78 });

            TestMathValueRoundTrip<TFormat>(RectF{ -1.0f, -2.0f, 3.0f, 4.0f });
            TestMathValueRoundTrip<TFormat>(RectInt{ -10, -20, 30, 40 });
            TestMathValueRoundTrip<TFormat>(RectUInt{ 10, 20, 30, 40 });

            TestMathValueRoundTrip<TFormat>(Vector3{ 1.0f, -2.0f, 3.5f });
            TestMathValueRoundTrip<TFormat>(PackedVector3F{ -4.0f, 5.0f, 6.25f });
            TestMathValueRoundTrip<TFormat>(Vector3Int{ -7, 8, -9 });
            TestMathValueRoundTrip<TFormat>(PackedVector3Int{ 10, -11, 12 });
            TestMathValueRoundTrip<TFormat>(Vector3UInt{ 13, 14, 0xf0000000u });
            TestMathValueRoundTrip<TFormat>(PackedVector3UInt{ 16, 17, 18 });

            TestMathValueRoundTrip<TFormat>(Vector4{ 1.0f, 2.0f, 3.0f, 4.0f });
            TestMathValueRoundTrip<TFormat>(PackedVector4F{ -1.0f, -2.0f, -3.0f, -4.0f });
            TestMathValueRoundTrip<TFormat>(Quaternion{ 0.1f, 0.2f, 0.3f, 0.9f });
            TestMathValueRoundTrip<TFormat>(Color4F{ 0.2f, 0.4f, 0.6f, 0.8f });

            TestMathValueRoundTrip<TFormat>(
                Matrix4x4::FromRows({ 1, 2, 3, 4 }, { 5, 6, 7, 8 }, { -1, -2, -3, -4 }, { -5, -6, -7, -8 }));
            TestMathValueRoundTrip<TFormat>(Aabb{ Vector3{ -1, -2, -3 }, Vector3{ 4, 5, 6 } });
            TestMathValueRoundTrip<TFormat>(PackedAabb{ PackedVector3F{ -7, -8, -9 }, PackedVector3F{ 10, 11, 12 } });
            TestMathValueRoundTrip<TFormat>(Sphere{ Vector3{ 1, 2, 3 }, 4.5f });
            TestMathValueRoundTrip<TFormat>(Obb{ Vector3{ 1, 2, 3 }, Vector3{ 4, 5, 6 }, Quaternion{ 0.1f, 0.2f, 0.3f, 0.9f } });
            TestMathValueRoundTrip<TFormat>(Transform::Create(Vector3{ 7, 8, 9 }, Quaternion{ 0.2f, 0.3f, 0.4f, 0.8f }, 2.5f));
        }
    } // namespace


    TEST(Serialization, PackedBinaryRoundTrip)
    {
        TestRoundTrip<PackedBinaryFormat>();
    }


    TEST(Serialization, BinaryErrorIncludesCodeAndOffset)
    {
        MemoryStream stream;
        const TestObject source = CreateObject();
        PackedBinaryFormat format;
        SerializationContext writer(&stream, format);
        ASSERT_EQ(writer.Store(source), ResultCode::kSuccess);

        auto bytes = stream.GetMutableData();
        ASSERT_GE(bytes.size(), 9);
        const std::byte firstByte = bytes[0];
        bytes[0] = std::byte{ 0 };

        stream.Rewind();
        TestObject destination;
        DeserializationContext reader(&stream, format);
        EXPECT_EQ(reader.Load(destination), ResultCode::kSchemaMismatch);
        EXPECT_EQ(reader.GetError().m_code, ResultCode::kSchemaMismatch);
        EXPECT_EQ(reader.GetError().m_byteOffset, 8);

        bytes[0] = firstByte;
        stream.Rewind();
        EXPECT_EQ(reader.Load(destination), ResultCode::kSuccess);
        EXPECT_EQ(reader.GetError().m_code, ResultCode::kSuccess);
    }


    TEST(Serialization, TaggedBinaryRoundTrip)
    {
        TestRoundTrip<TaggedBinaryFormat>();
    }


    TEST(Serialization, MathValuesPackedBinaryRoundTrip)
    {
        TestMathValuesRoundTrip<PackedBinaryFormat>();
    }


    TEST(Serialization, MathValuesTaggedBinaryRoundTrip)
    {
        TestMathValuesRoundTrip<TaggedBinaryFormat>();
    }


    TEST(Serialization, MathValuesJsonRoundTrip)
    {
        TestMathValuesRoundTrip<JsonFormat>();
    }


    TEST(Serialization, ManualSerializationWithoutMacrosOrCodegen)
    {
        MemoryStream stream;
        const ManualObject source{ .m_count = 42, .m_name = "manual" };
        TaggedBinaryFormat format;
        SerializationContext writer(&stream, format);
        ASSERT_EQ(writer.Store(source), ResultCode::kSuccess);

        stream.Rewind();
        ManualObject destination;
        DeserializationContext reader(&stream, format);
        ASSERT_EQ(reader.Load(destination), ResultCode::kSuccess);
        EXPECT_EQ(destination.m_count, source.m_count);
        EXPECT_EQ(destination.m_name, source.m_name);
        EXPECT_EQ(reader.GetSerializedVersion(), ManualObject::kVersion);
    }


    TEST(Serialization, TaggedBinarySupportsSchemaEvolution)
    {
        struct OldValue final
        {
            uint32_t m_common = 0;
            uint32_t m_removed = 0;
        };

        struct NewValue final
        {
            uint32_t m_common = 0;
            uint32_t m_added = 17;
        };

        Rtti::Type oldType;
        oldType.m_id = TestObject::TypeID;
        oldType.m_serializationVersion = 1;
        oldType.m_serializationSchemaHash = 11;
        oldType.m_serialize = [](SerializationContext& context, const void* value) {
            const auto& typedValue = *static_cast<const OldValue*>(value);
            if (auto object = context.BeginObject())
            {
                object.Field("m_common", typedValue.m_common).Field("m_removed", typedValue.m_removed);
                return context.GetResultCode();
            }

            return context.GetResultCode();
        };

        Rtti::Type newType;
        newType.m_id = oldType.m_id;
        newType.m_serializationVersion = 2;
        newType.m_serializationSchemaHash = 22;
        newType.m_deserialize = [](DeserializationContext& context, void* value) {
            auto& typedValue = *static_cast<NewValue*>(value);
            if (auto object = context.BeginObject())
            {
                object.Field("m_common", typedValue.m_common).Field("m_added", typedValue.m_added);
                return context.GetResultCode();
            }

            return context.GetResultCode();
        };

        MemoryStream stream;
        const OldValue source{ .m_common = 42, .m_removed = 99 };
        TaggedBinaryFormat format;
        SerializationContext writer(&stream, format);
        ASSERT_EQ(writer.Store(oldType, &source), ResultCode::kSuccess);

        stream.Rewind();
        NewValue destination;
        DeserializationContext reader(&stream, format);
        ASSERT_EQ(reader.Load(newType, &destination), ResultCode::kSuccess);
        EXPECT_EQ(destination.m_common, source.m_common);
        EXPECT_EQ(destination.m_added, 17);
        EXPECT_EQ(reader.GetSerializedVersion(), 1);
        EXPECT_EQ(reader.GetSerializedSchemaHash(), 11);
    }


    TEST(Serialization, JsonRoundTrip)
    {
        MemoryStream stream;
        TestObject source = CreateObject();
        JsonFormat format;
        SerializationContext writer(&stream, format);
        ASSERT_EQ(writer.Store(source), ResultCode::kSuccess);

        const auto json = stream.GetData();
        const std::string_view text{ reinterpret_cast<const char*>(json.data()), json.size() };
        ASSERT_GT(text.size(), 0);

        stream.Rewind();
        TestObject destination;
        DeserializationContext reader(&stream, format);
        ASSERT_EQ(reader.Load(destination), ResultCode::kSuccess);
        ExpectEqual(source, destination);
    }


    TEST(Serialization, JsonEmptyValuesRoundTrip)
    {
        MemoryStream stream;
        TestObject source = CreateObject();
        source.m_name.clear();
        source.m_values.clear();

        JsonFormat format;
        SerializationContext writer(&stream, format);
        ASSERT_EQ(writer.Store(source), ResultCode::kSuccess);

        stream.Rewind();
        TestObject destination;
        DeserializationContext reader(&stream, format);
        ASSERT_EQ(reader.Load(destination), ResultCode::kSuccess);
        ExpectEqual(source, destination);
    }


    TEST(Serialization, JsonParseErrorIncludesLineAndColumn)
    {
        MemoryStream stream;
        static constexpr char kInvalidJson[] = "{\n  \"$type\": ]\n}";
        ASSERT_EQ(stream.WriteFromBuffer(kInvalidJson, sizeof(kInvalidJson) - 1), sizeof(kInvalidJson) - 1);
        stream.Rewind();

        TestObject destination;
        JsonFormat format;
        DeserializationContext reader(&stream, format);
        EXPECT_EQ(reader.Load(destination), ResultCode::kJsonParseError);
        EXPECT_EQ(reader.GetError().m_code, ResultCode::kJsonParseError);
        EXPECT_EQ(reader.GetError().m_line, 2);
        EXPECT_GT(reader.GetError().m_column, 1);
    }


    TEST(Serialization, DynamicCallbacksAndLifecycle)
    {
        const Rtti::Type& type = Rtti::GetType<TestObject>();
        ASSERT_NE(type.m_defaultConstructor, nullptr);
        ASSERT_NE(type.m_destructor, nullptr);
        ASSERT_NE(type.m_serialize, nullptr);
        ASSERT_NE(type.m_deserialize, nullptr);
        EXPECT_EQ(type.m_serializationVersion, TestObject::kVersion);
        EXPECT_NE(type.m_serializationSchemaHash, 0);

        void* storage = Memory::DefaultAllocate(type.m_size, type.m_alignment);
        type.m_defaultConstructor(storage);

        MemoryStream stream;
        const TestObject source = CreateObject();
        TaggedBinaryFormat format;
        SerializationContext writer(&stream, format);
        ASSERT_EQ(writer.Store(type, &source), ResultCode::kSuccess);

        stream.Rewind();
        DeserializationContext reader(&stream, format);
        ASSERT_EQ(reader.Load(type, storage), ResultCode::kSuccess);
        ExpectEqual(source, *static_cast<TestObject*>(storage));

        type.m_destructor(storage);
        Memory::DefaultFree(storage);
    }


    TEST(Serialization, IncompleteReflectionMetadataIsAllowed)
    {
        const Rtti::Type& type = Rtti::GetType<TestObject>();
        ASSERT_EQ(type.m_fields.size(), 6);
        EXPECT_EQ(type.m_fields[2].m_type, Rtti::TypeID::kNull);

        const Rtti::Type& descType = Rtti::GetType<PackedDesc>();
        ASSERT_EQ(descType.m_fields.size(), 1);
        EXPECT_EQ(descType.m_fields[0].m_name, "m_scale");
    }
} // namespace FE::Serialization::Tests
