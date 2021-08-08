#include <FeCore/Console/FeLog.h>
#include <FeCore/Strings/String.h>
#include <gtest/gtest.h>

TEST(Strings, EmptySizeCapacity)
{
    FE::String str;
    ASSERT_EQ(str.Capacity(), 22);
    ASSERT_EQ(str.Size(), 0);
}

TEST(Strings, SmallSizeCapacity)
{
    FE::String str = "q";
    ASSERT_EQ(str.Capacity(), 22);
    ASSERT_EQ(str.Size(), 1);
}

TEST(Strings, LongSizeCapacity)
{
    const char* cstr = "loooooooooooooooooooooooooooooooooooooooooong";
    FE::String str   = cstr;
    ASSERT_GT(str.Capacity(), 22);
    ASSERT_EQ(str.Size(), strlen(cstr));
}

TEST(Strings, SmallCodepointAt)
{
    const char* utf8 = u8"qЯwgЫЧ";
    ASSERT_TRUE(FE::UTF8::Valid(utf8));

    // see https://developercommunity.visualstudio.com/t/incorrect-parsing-of-string-literals-prefixed-with/877508
#if !defined FE_COMPILER_MSVC
    FE::String str = utf8;
    EXPECT_EQ(str.CodePointAt(0), L'q');
    EXPECT_EQ(str.CodePointAt(4), L'Ы');
#endif
}

TEST(Strings, LongCodepointAt)
{
    const char* utf8 = u8"loooooooooooooooooooooooooooooooooooong qЯwgЫЧ";
    ASSERT_TRUE(FE::UTF8::Valid(utf8));

#if !defined FE_COMPILER_MSVC
    FE::String str = utf8;
    EXPECT_EQ(str.CodePointAt(40), L'q');
    EXPECT_EQ(str.CodePointAt(44), L'Ы');
#endif
}

TEST(Strings, Equals)
{
    FE::String a = "abc";
    FE::String b = "abc";
    FE::String c = "xyz";
    FE::String d = "qqqq";

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(a, d);
}

TEST(Strings, Slice)
{
    FE::String str = "===slice===";
    ASSERT_EQ(str(3, 8), "slice");
}

TEST(Strings, SmallConcat)
{
    FE::String a = "A";
    FE::String b = "B";
    ASSERT_EQ(a + b, "AB");
}

TEST(Strings, LongConcat)
{
    FE::String a(128, 'A');
    FE::String b(128, 'B');
    auto c = a + b;
    ASSERT_EQ(c(0, 128), a);
    ASSERT_EQ(c(128, 256), b);
}
