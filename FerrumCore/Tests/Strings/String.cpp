#include <FeCore/Console/FeLog.h>
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
    ASSERT_GE(str.Capacity(), 35);
    ASSERT_EQ(str.Size(), strlen(cstr));
}

TEST(Strings, SmallByteAt)
{
    FE::String str = "0123456789";
    EXPECT_EQ(str.ByteAt(0), '0');
    EXPECT_EQ(str.ByteAt(9), '9');
}

TEST(Strings, LongByteAt)
{
    FE::String str = "loooooooooooooooooooooooooooooooooooong 0123456789";
    EXPECT_EQ(str.ByteAt(40), '0');
    EXPECT_EQ(str.ByteAt(49), '9');
}

TEST(Strings, Length)
{
    FE::String smalls = "0123";
    FE::String longs  = "loooooooooooooooooooooooooooooooooooong";
    EXPECT_EQ(smalls.Length(), 4);
    EXPECT_EQ(longs.Length(), 39);
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

TEST(Strings, ShrinkReserve)
{
    FE::String str;
    EXPECT_EQ(str.Capacity(), 22); // initially small

    str.Reserve(128); // small -> long
    EXPECT_GE(str.Capacity(), 128);

    str.Append("123");
    str.Shrink(); // long -> small
    EXPECT_EQ(str.Capacity(), 22);

    const char l[] = "looooooooooooooooooooooooooooooooooooong";
    str.Append(l); // small -> long
    str.Shrink();  // long -> long
    EXPECT_GE(str.Capacity(), sizeof(l) + 3 - 1);
}

TEST(Strings, Compare)
{
    EXPECT_EQ(FE::String("abc").Compare(FE::StringSlice("abc")), 0);
    EXPECT_EQ(FE::String("abc").Compare(FE::String("abc")), 0);

    EXPECT_EQ(FE::String("abc").Compare("abc"), 0);

    EXPECT_GT(FE::String("abcd").Compare("abc"), 0);
    EXPECT_LT(FE::String("abc").Compare("abcd"), 0);

    EXPECT_GT(FE::String("az").Compare("aa"), 0);
    EXPECT_LT(FE::String("aa").Compare("az"), 0);

    EXPECT_GT(FE::String("azz").Compare("aa"), 0);
    EXPECT_GT(FE::String("az").Compare("aaz"), 0);
    EXPECT_GT(FE::String("aza").Compare("aa"), 0);
    EXPECT_GT(FE::String("az").Compare("aaa"), 0);
}

TEST(Strings, Split)
{
    auto str = FE::String("abc def 123");
    auto split = str.Split();
    ASSERT_EQ(split.Size(), 3);
    EXPECT_EQ(split[0], FE::StringSlice("abc"));
    EXPECT_EQ(split[1], FE::StringSlice("def"));
    EXPECT_EQ(split[2], FE::StringSlice("123"));
}
