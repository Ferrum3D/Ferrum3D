#include <FeCore/Logging/Trace.h>
#include <festd/string.h>
#include <gtest/gtest.h>

using namespace FE;

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
