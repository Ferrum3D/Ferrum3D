#include <FeCore/Logging/Trace.h>
#include <festd/string.h>
#include <gtest/gtest.h>

using namespace FE;

namespace
{
    class TrackingMemoryResource final : public std::pmr::memory_resource
    {
    public:
        uint32_t m_allocations = 0;
        uint32_t m_deallocations = 0;
        size_t m_allocatedBytes = 0;
        size_t m_deallocatedBytes = 0;

    private:
        void* do_allocate(size_t bytes, size_t alignment) override
        {
            ++m_allocations;
            m_allocatedBytes += bytes;
            return std::pmr::new_delete_resource()->allocate(bytes, alignment);
        }

        void do_deallocate(void* pointer, size_t bytes, size_t alignment) override
        {
            ++m_deallocations;
            m_deallocatedBytes += bytes;
            std::pmr::new_delete_resource()->deallocate(pointer, bytes, alignment);
        }

        bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
        {
            return this == &other;
        }
    };
} // namespace

TEST(Strings, EmptySizeCapacity)
{
    festd::string str;
    ASSERT_EQ(str.capacity(), 23);
    ASSERT_EQ(str.size(), 0);
}

TEST(Strings, SmallSizeCapacity)
{
    festd::string str = "q";
    ASSERT_EQ(str.capacity(), 23);
    ASSERT_EQ(str.size(), 1);
}

TEST(Strings, LongSizeCapacity)
{
    const char* cstr = "loooooooooooooooooooooooooooooooooooooooooong";
    festd::string str = cstr;
    ASSERT_GE(str.capacity(), 35);
    ASSERT_EQ(str.size(), strlen(cstr));
}

TEST(Strings, MoveConstruct)
{
    const char* cstr = "loooooooooooooooooooooooooooooooooooooooooong";
    festd::string str1 = cstr;
    const char* data = str1.data();
    festd::string str2 = std::move(str1);
    ASSERT_EQ(ASCII::Compare(str2.data(), cstr), 0);
    ASSERT_EQ(str2.data(), data);
}

TEST(Strings, MoveAssign)
{
    const char* cstr = "loooooooooooooooooooooooooooooooooooooooooong";
    festd::string str1 = cstr;
    festd::string str2 = "other string that already owns overflow memory";
    const char* data = str1.data();

    str2 = std::move(str1);

    ASSERT_EQ(ASCII::Compare(str2.data(), cstr), 0);
    ASSERT_EQ(str2.data(), data);
    EXPECT_EQ(str1.size(), 0);
    EXPECT_STREQ(str1.c_str(), "");
}

TEST(Strings, StringViewConversion)
{
    const festd::string s1{ festd::string_view{ "test" } };
    EXPECT_EQ(s1, "test");

    const festd::string_view s2 = s1;
    EXPECT_EQ(s2, "test");
    EXPECT_EQ(s2.data(), s1.data());

    const festd::string s3 = s1;
    EXPECT_EQ(s3, "test");
    EXPECT_NE(s3.data(), s1.data());
}

TEST(Strings, EmptyStringViewHasNullData)
{
    const festd::string_view view;

    EXPECT_EQ(view.data(), nullptr);
    EXPECT_EQ(view.size(), 0);
}

TEST(Strings, EmptyStringViewOperationsTolerateNullData)
{
    const festd::string_view empty;
    const festd::string str = "abc";

    EXPECT_EQ(empty, festd::string_view{});
    EXPECT_EQ(empty, "");
    EXPECT_EQ("", empty);
    EXPECT_EQ(empty.compare(festd::string_view{}), 0);
    EXPECT_TRUE(str.starts_with(empty));
    EXPECT_TRUE(str.ends_with(empty));
    EXPECT_EQ(str.find(empty), str.begin());
    EXPECT_EQ(DefaultHash(empty), DefaultHash(nullptr, 0));
}

TEST(Strings, SmallByteAt)
{
    festd::string str = "0123456789";
    EXPECT_EQ(str.byte_at(0), '0');
    EXPECT_EQ(str.byte_at(9), '9');
}

TEST(Strings, LongByteAt)
{
    festd::string str = "loooooooooooooooooooooooooooooooooooong 0123456789";
    EXPECT_EQ(str.byte_at(40), '0');
    EXPECT_EQ(str.byte_at(49), '9');
}

TEST(Strings, Length)
{
    festd::string smalls = "0123";
    festd::string longs = "loooooooooooooooooooooooooooooooooooong";
    EXPECT_EQ(smalls.length(), 4);
    EXPECT_EQ(longs.length(), 39);
}

TEST(Strings, SmallCodepointAt)
{
    const char* utf8 = "qЯwgЫЧ";
    ASSERT_TRUE(UTF8::IsValid(utf8));

    festd::string str = utf8;
    EXPECT_EQ(str.codepoint_at(0), L'q');
    EXPECT_EQ(str.codepoint_at(3), L'g');
    EXPECT_EQ(str.codepoint_at(4), L'Ы');
}

TEST(Strings, LongCodepointAt)
{
    const char* utf8 = "loooooooooooooooooooooooooooooooooooong qЯwgЫЧ";
    ASSERT_TRUE(UTF8::IsValid(utf8));

    festd::string str = utf8;
    EXPECT_EQ(str.codepoint_at(40), L'q');
    EXPECT_EQ(str.codepoint_at(44), L'Ы');
}

TEST(Strings, Equals)
{
    festd::string a = "abc";
    festd::string b = "abc";
    festd::string c = "xyz";
    festd::string d = "qqqq";
    festd::string e = "";
    festd::string f;

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(a, d);
    EXPECT_EQ(e, f);
    EXPECT_NE(a, e);
    EXPECT_NE(d, f);
}

TEST(Strings, Slice)
{
    festd::string str = "===slice===";
    ASSERT_EQ(str.substr(3, 5), "slice");
    ASSERT_EQ(str.substr_ascii(3, 5), "slice");

    str = "===подстрока===";
    ASSERT_EQ(str.substr(3, 9), "подстрока");
}

TEST(Strings, SmallConcat)
{
    festd::string a = "A";
    festd::string b = "B";
    ASSERT_EQ(a + b, "AB");
    ASSERT_EQ(a + festd::string_view{ "B" }, "AB");
}

TEST(Strings, LongConcat)
{
    festd::string a(128, 'A');
    festd::string b(128, 'B');
    auto c = a + b;
    ASSERT_EQ(c.substr(0, 128), a);
    ASSERT_EQ(c.substr(128, 128), b);
}

TEST(Strings, ShrinkReserve)
{
    festd::string str;
    EXPECT_EQ(str.capacity(), 23); // initially small

    str.reserve(128); // small -> long
    EXPECT_GE(str.capacity(), 128);

    str.append("123");
    str.shrink_to_fit(); // long -> small
    EXPECT_EQ(str.capacity(), 23);

    const char l[] = "looooooooooooooooooooooooooooooooooooong";
    str.append(l);       // small -> long
    str.shrink_to_fit(); // long -> long
    EXPECT_GE(str.capacity(), sizeof(l) + 3 - 1);
}

TEST(Strings, ResizeUpdatesCStringTerminator)
{
    festd::string str = "abcdef";

    str.resize(2, 'x');
    EXPECT_EQ(str.size(), 2);
    EXPECT_STREQ(str.c_str(), "ab");

    str.resize(5, 'x');
    EXPECT_EQ(str.size(), 5);
    EXPECT_STREQ(str.c_str(), "abxxx");
}

TEST(Strings, ClearUpdatesCStringTerminator)
{
    festd::string str = "abcdef";

    str.clear();

    EXPECT_EQ(str.size(), 0);
    EXPECT_STREQ(str.c_str(), "");

    festd::string longStr(32, 'a');
    longStr.clear();

    EXPECT_EQ(longStr.size(), 0);
    EXPECT_STREQ(longStr.c_str(), "");
}

TEST(Strings, DynamicReservePreservesCStringTerminator)
{
    festd::string str = "abc";

    str.reserve(128);

    EXPECT_EQ(str.size(), 3);
    EXPECT_STREQ(str.c_str(), "abc");
}

TEST(Strings, DynamicShrinkToShortFreesOverflowStorage)
{
    TrackingMemoryResource allocator;

    {
        festd::pmr::string str{ &allocator };
        str.reserve(128);
        ASSERT_EQ(allocator.m_allocations, 1);

        str.assign("abc", 3);
        str.shrink_to_fit();

        EXPECT_EQ(str.capacity(), 23);
        EXPECT_EQ(allocator.m_deallocations, 1);
        EXPECT_EQ(allocator.m_allocatedBytes, allocator.m_deallocatedBytes);
        EXPECT_STREQ(str.c_str(), "abc");
    }

    EXPECT_EQ(allocator.m_allocations, allocator.m_deallocations);
}

TEST(Strings, DynamicShrinkToLongUsesOriginalAllocationSize)
{
    TrackingMemoryResource allocator;

    {
        festd::pmr::string str{ &allocator };
        str.reserve(128);
        const char* value = "loooooooooooooooooooooooooooooooooooooooooong";
        str.assign(value, static_cast<uint32_t>(strlen(value)));

        str.shrink_to_fit();

        EXPECT_EQ(str, value);
        EXPECT_EQ(allocator.m_allocations, 2);
        EXPECT_EQ(allocator.m_deallocations, 1);
        EXPECT_LT(allocator.m_deallocatedBytes, allocator.m_allocatedBytes);
    }

    EXPECT_EQ(allocator.m_allocations, allocator.m_deallocations);
    EXPECT_EQ(allocator.m_allocatedBytes, allocator.m_deallocatedBytes);
}

TEST(Strings, FixedStringExactCapacityHasTerminator)
{
    festd::basic_fixed_string<4> str = "1234";

    EXPECT_EQ(str.size(), 4);
    EXPECT_EQ(str.capacity(), 4);
    EXPECT_STREQ(str.c_str(), "1234");
}

TEST(Strings, InlineStringUsesInlineStorage)
{
    festd::basic_inline_string<4> str = "1234";
    EXPECT_EQ(str.capacity(), 4);
    EXPECT_EQ(str.size(), 4);
    EXPECT_EQ(str, "1234");
    EXPECT_STREQ(str.c_str(), "1234");
}

TEST(Strings, InlineStringGrowsPastInlineStorage)
{
    festd::basic_inline_string<4> str = "1234";
    const char* inlineData = str.data();

    str.push_back('5');

    EXPECT_GT(str.capacity(), 4);
    EXPECT_NE(str.data(), inlineData);
    EXPECT_EQ(str, "12345");
}

TEST(Strings, InlineStringShrinkReturnsToInlineStorage)
{
    festd::basic_inline_string<4> str = "12345";
    const char* heapData = str.data();

    str.assign("123", 3);
    str.shrink_to_fit();

    EXPECT_EQ(str.capacity(), 4);
    EXPECT_NE(str.data(), heapData);
    EXPECT_EQ(str, "123");
}

TEST(Strings, InlineStringMovePreservesValue)
{
    festd::basic_inline_string<4> str = "12345";

    festd::basic_inline_string<4> moved = std::move(str);

    EXPECT_EQ(moved, "12345");
    EXPECT_EQ(str.size(), 0);
    EXPECT_STREQ(str.c_str(), "");
}

TEST(Strings, InlineStringSelfAppendCanGrow)
{
    festd::basic_inline_string<4> str = "1234";
    str += festd::string_view{ str };

    EXPECT_EQ(str, "12341234");
    EXPECT_STREQ(str.c_str(), "12341234");
}

TEST(Strings, AssignFromOwnRange)
{
    festd::string str = "abcdef";

    str.assign(str.data() + 1, 3);

    EXPECT_EQ(str, "bcd");
    EXPECT_STREQ(str.c_str(), "bcd");
}

TEST(Strings, DynamicSelfAppendCanGrow)
{
    festd::string str(32, 'a');
    str += festd::string_view{ str };

    ASSERT_EQ(str.size(), 64);
    EXPECT_EQ(str.substr(0, 32), festd::string(32, 'a'));
    EXPECT_EQ(str.substr(32, 32), festd::string(32, 'a'));
}

TEST(Strings, FindLongerNeedleReturnsEnd)
{
    const festd::string str = "abc";

    EXPECT_EQ(str.find("abcd"), str.end());
}

TEST(Strings, FindLastOfEmptyStringReturnsEnd)
{
    const festd::string str;

    EXPECT_EQ(str.find_last_of('x'), str.end());
}

TEST(Strings, IteratorNegativeOffset)
{
    const festd::string str = "abc";

    EXPECT_EQ(*(str.end() - 1), 'c');
}

TEST(Strings, SubstrPastEndReturnsEmpty)
{
    const festd::string str = "abc";

    EXPECT_EQ(str.substr(8), festd::string_view{});
    EXPECT_EQ(str.substr_ascii(8), festd::ascii_view{});
}

TEST(Strings, PmrCopyAssignmentUsesSourceAllocator)
{
    TrackingMemoryResource allocatorA;
    TrackingMemoryResource allocatorB;

    {
        festd::pmr::string a{ &allocatorA };
        const char* valueA = "loooooooooooooooooooooooooooooooooooooooooong";
        a.assign(valueA, static_cast<uint32_t>(strlen(valueA)));

        festd::pmr::string b{ &allocatorB };
        const char* valueB = "boooooooooooooooooooooooooooooooooooooooooong";
        b.assign(valueB, static_cast<uint32_t>(strlen(valueB)));

        a = b;

        EXPECT_EQ(a.get_allocator(), &allocatorB);
        EXPECT_EQ(a, b);
        EXPECT_EQ(allocatorA.m_deallocations, 1);
    }

    EXPECT_EQ(allocatorA.m_allocations, allocatorA.m_deallocations);
    EXPECT_EQ(allocatorB.m_allocations, allocatorB.m_deallocations);
}

TEST(Strings, PmrSetAllocatorRehomesStorage)
{
    TrackingMemoryResource allocatorA;
    TrackingMemoryResource allocatorB;

    {
        festd::pmr::string str{ &allocatorA };
        const char* value = "loooooooooooooooooooooooooooooooooooooooooong";
        str.assign(value, static_cast<uint32_t>(strlen(value)));

        str.set_allocator(&allocatorB);

        EXPECT_EQ(str.get_allocator(), &allocatorB);
        EXPECT_EQ(str, value);
        EXPECT_EQ(allocatorA.m_deallocations, 1);
    }

    EXPECT_EQ(allocatorA.m_allocations, allocatorA.m_deallocations);
    EXPECT_EQ(allocatorB.m_allocations, allocatorB.m_deallocations);
}

TEST(Strings, PmrMoveConstructStealsStorageWithoutAllocation)
{
    TrackingMemoryResource allocator;
    const char* value = "loooooooooooooooooooooooooooooooooooooooooong";

    {
        festd::pmr::string source{ &allocator };
        source.assign(value, static_cast<uint32_t>(strlen(value)));
        const uint32_t allocations = allocator.m_allocations;
        const char* data = source.data();

        festd::pmr::string moved{ std::move(source) };

        EXPECT_EQ(allocator.m_allocations, allocations);
        EXPECT_EQ(moved.get_allocator(), &allocator);
        EXPECT_EQ(moved.data(), data);
        EXPECT_EQ(moved, value);
        EXPECT_EQ(source.size(), 0);
        EXPECT_STREQ(source.c_str(), "");
    }

    EXPECT_EQ(allocator.m_allocations, allocator.m_deallocations);
}

TEST(Strings, PmrMoveAssignStealsStorageWithoutAllocation)
{
    TrackingMemoryResource allocatorA;
    TrackingMemoryResource allocatorB;
    const char* value = "loooooooooooooooooooooooooooooooooooooooooong";

    {
        festd::pmr::string source{ &allocatorA };
        source.assign(value, static_cast<uint32_t>(strlen(value)));

        festd::pmr::string destination{ &allocatorB };
        const char* otherValue = "other string that already owns overflow memory";
        destination.assign(otherValue, static_cast<uint32_t>(strlen(otherValue)));

        const uint32_t allocationsA = allocatorA.m_allocations;
        const uint32_t allocationsB = allocatorB.m_allocations;
        const char* data = source.data();

        destination = std::move(source);

        EXPECT_EQ(allocatorA.m_allocations, allocationsA);
        EXPECT_EQ(allocatorB.m_allocations, allocationsB);
        EXPECT_EQ(allocatorB.m_deallocations, 1);
        EXPECT_EQ(destination.get_allocator(), &allocatorA);
        EXPECT_EQ(destination.data(), data);
        EXPECT_EQ(destination, value);
        EXPECT_EQ(source.size(), 0);
        EXPECT_STREQ(source.c_str(), "");
    }

    EXPECT_EQ(allocatorA.m_allocations, allocatorA.m_deallocations);
    EXPECT_EQ(allocatorB.m_allocations, allocatorB.m_deallocations);
}

TEST(Strings, Compare)
{
    EXPECT_EQ(festd::string{}.compare(festd::string{}), 0);

    EXPECT_EQ(festd::string("abc").compare(festd::string_view("abc")), 0);
    EXPECT_EQ(festd::string("abc").compare(festd::string("abc")), 0);

    EXPECT_EQ(festd::string("abc").compare("abc"), 0);

    EXPECT_GT(festd::string("abcd").compare("abc"), 0);
    EXPECT_LT(festd::string("abc").compare("abcd"), 0);

    EXPECT_GT(festd::string("az").compare("aa"), 0);
    EXPECT_LT(festd::string("aa").compare("az"), 0);

    EXPECT_GT(festd::string("azz").compare("aa"), 0);
    EXPECT_GT(festd::string("az").compare("aaz"), 0);
    EXPECT_GT(festd::string("aza").compare("aa"), 0);
    EXPECT_GT(festd::string("az").compare("aaa"), 0);
}

TEST(Strings, Strip)
{
    EXPECT_EQ(festd::string("123").strip(), festd::string_view("123"));
    EXPECT_EQ(festd::string("    123    ").strip_right(), festd::string_view("    123"));
    EXPECT_EQ(festd::string("    123    ").strip_left(), festd::string_view("123    "));
    EXPECT_EQ(festd::string(" \t \n \r   ").strip_left(), festd::string_view{});
}

TEST(Strings, StartsWith)
{
    EXPECT_TRUE(festd::string("").starts_with(""));
    EXPECT_TRUE(festd::string("1234").starts_with(""));
    EXPECT_TRUE(festd::string("1234").starts_with("1"));
    EXPECT_TRUE(festd::string("1234").starts_with("12"));
    EXPECT_TRUE(festd::string("1234").starts_with("1234"));
    EXPECT_FALSE(festd::string("1234").starts_with("21"));
    EXPECT_FALSE(festd::string("1234").starts_with("12345"));
}

TEST(Strings, EndsWith)
{
    EXPECT_TRUE(festd::string("").ends_with(""));
    EXPECT_TRUE(festd::string("1234").ends_with(""));
    EXPECT_TRUE(festd::string("1234").ends_with("4"));
    EXPECT_TRUE(festd::string("1234").ends_with("34"));
    EXPECT_TRUE(festd::string("1234").ends_with("1234"));
    EXPECT_FALSE(festd::string("1234").ends_with("21"));
    EXPECT_FALSE(festd::string("1234").ends_with("12345"));
}

TEST(Strings, NameConversion)
{
    const Env::Name name = "test name";
    const Env::Name other{ festd::string_view{ "test name" } };

    EXPECT_EQ(name, Env::Name{ "test name" });
    EXPECT_EQ(name, other);
    EXPECT_EQ(name, festd::string_view{ "test name" });
    EXPECT_EQ(festd::ascii_view{ name }, festd::string_view{ "test name" });
    EXPECT_EQ(festd::ascii_view{ name }, festd::string{ "test name" });
}
