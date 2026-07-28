#include <Core/IO/StreamBase.h>
#include <Core/Serialization/BinarySerialization.h>
#include <Core/Serialization/JsonSerialization.h>
#include <Serialization/SerializationTypes.h>
#include <gtest/gtest.h>

namespace FE::Serialization::Tests
{
    namespace
    {
        class MemoryStream final : public IO::BufferedStream
        {
        public:
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


        template<class TContext>
        void TestRoundTrip()
        {
            MemoryStream stream;
            TestObject source = CreateObject();
            TContext writer(&stream);
            ASSERT_TRUE(writer.Store(source));

            stream.Rewind();
            TestObject destination;
            destination.m_transient = 99;
            TContext reader(&stream);
            ASSERT_TRUE(reader.Load(destination));
            ExpectEqual(source, destination);
            EXPECT_EQ(destination.m_transient, 99);
        }
    } // namespace


    TEST(Serialization, PackedBinaryRoundTrip)
    {
        TestRoundTrip<PackedBinaryContext>();
    }


    TEST(Serialization, BinaryErrorIncludesCodeAndOffset)
    {
        MemoryStream stream;
        const TestObject source = CreateObject();
        PackedBinaryContext writer(&stream);
        ASSERT_TRUE(writer.Store(source));

        auto bytes = stream.GetMutableData();
        ASSERT_GE(bytes.size(), 9);
        bytes[0] = std::byte{ 0 };

        stream.Rewind();
        TestObject destination;
        PackedBinaryContext reader(&stream);
        EXPECT_FALSE(reader.Load(destination));
        EXPECT_EQ(reader.GetError().m_code, ErrorCode::kInvalidHeader);
        EXPECT_EQ(reader.GetError().m_byteOffset, 32);
    }


    TEST(Serialization, TaggedBinaryRoundTrip)
    {
        TestRoundTrip<TaggedBinaryContext>();
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
            auto object = context.BeginObject();
            if (!object)
                return false;
            object.Field("m_common", typedValue.m_common).Field("m_removed", typedValue.m_removed);
            return context.IsValid();
        };

        Rtti::Type newType;
        newType.m_id = oldType.m_id;
        newType.m_serializationVersion = 2;
        newType.m_serializationSchemaHash = 22;
        newType.m_deserialize = [](SerializationContext& context, void* value) {
            auto& typedValue = *static_cast<NewValue*>(value);
            auto object = context.BeginObject();
            if (!object)
                return false;
            object.Field("m_common", typedValue.m_common).Field("m_added", typedValue.m_added);
            return context.IsValid();
        };

        MemoryStream stream;
        const OldValue source{ .m_common = 42, .m_removed = 99 };
        TaggedBinaryContext writer(&stream);
        ASSERT_TRUE(writer.Store(oldType, &source));

        stream.Rewind();
        NewValue destination;
        TaggedBinaryContext reader(&stream);
        ASSERT_TRUE(reader.Load(newType, &destination));
        EXPECT_EQ(destination.m_common, source.m_common);
        EXPECT_EQ(destination.m_added, 17);
        EXPECT_EQ(reader.GetSerializedVersion(), 1);
        EXPECT_EQ(reader.GetSerializedSchemaHash(), 11);
    }


    TEST(Serialization, JsonRoundTrip)
    {
        MemoryStream stream;
        TestObject source = CreateObject();
        JsonContext writer(&stream);
        ASSERT_TRUE(writer.Store(source));

        const auto json = stream.GetData();
        const std::string_view text{ reinterpret_cast<const char*>(json.data()), json.size() };
        EXPECT_NE(text.find("\"0x1.f7cedap-4\""), std::string_view::npos);

        stream.Rewind();
        TestObject destination;
        JsonContext reader(&stream);
        ASSERT_TRUE(reader.Load(destination));
        ExpectEqual(source, destination);
    }


    TEST(Serialization, JsonEmptyValuesRoundTrip)
    {
        MemoryStream stream;
        TestObject source = CreateObject();
        source.m_name.clear();
        source.m_values.clear();

        JsonContext writer(&stream);
        ASSERT_TRUE(writer.Store(source));

        stream.Rewind();
        TestObject destination;
        JsonContext reader(&stream);
        ASSERT_TRUE(reader.Load(destination));
        ExpectEqual(source, destination);
    }


    TEST(Serialization, JsonParseErrorIncludesLineAndColumn)
    {
        MemoryStream stream;
        static constexpr char kInvalidJson[] = "{\n  \"$type\": ]\n}";
        ASSERT_EQ(stream.WriteFromBuffer(kInvalidJson, sizeof(kInvalidJson) - 1), sizeof(kInvalidJson) - 1);
        stream.Rewind();

        TestObject destination;
        JsonContext reader(&stream);
        EXPECT_FALSE(reader.Load(destination));
        EXPECT_EQ(reader.GetError().m_code, ErrorCode::kJsonParseError);
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
        EXPECT_EQ(type.m_serializationVersion, TestObject::kSerializationVersion);
        EXPECT_NE(type.m_serializationSchemaHash, 0);

        void* storage = Memory::DefaultAllocate(type.m_size, type.m_alignment);
        type.m_defaultConstructor(storage);

        MemoryStream stream;
        const TestObject source = CreateObject();
        TaggedBinaryContext writer(&stream);
        ASSERT_TRUE(writer.Store(type, &source));

        stream.Rewind();
        TaggedBinaryContext reader(&stream);
        ASSERT_TRUE(reader.Load(type, storage));
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
