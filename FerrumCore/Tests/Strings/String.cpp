#include <FeCore/Console/FeLog.h>
#include <gtest/gtest.h>

TEST(Strings, AssignmentFreesMemory)
{
    auto& alloc = FE::GlobalAllocator<FE::HeapAllocator>::Get();
    auto before = alloc.TotalAllocated();
    {
        FE::String str = "loooooooooooooooooooooooooooooooooooooooooong123";
        str            = FE::String("loooooooooooooooooooooooooooooooooooooooooong321");
    }
    EXPECT_EQ(before, alloc.TotalAllocated());
    {
        FE::String str1 = "loooooooooooooooooooooooooooooooooooooooooong123";
        FE::String str2 = "loooooooooooooooooooooooooooooooooooooooooong321";
        str2            = str1;
    }
    EXPECT_EQ(before, alloc.TotalAllocated());
    {
        FE::String str1 = "loooooooooooooooooooooooooooooooooooooooooong123";
        FE::String str2 = FE::String("loooooooooooooooooooooooooooooooooooooooooong321");
        str2            = str1;
    }
    EXPECT_EQ(before, alloc.TotalAllocated());
}

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
    const char* utf8 = "qЯwgЫЧ";
    ASSERT_TRUE(FE::UTF8::Valid(utf8));

    FE::String str = utf8;
    EXPECT_EQ(str.CodePointAt(0), L'q');
    EXPECT_EQ(str.CodePointAt(3), L'g');
    // EXPECT_EQ(str.CodePointAt(4), L'Ы');
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
    FE::String e = "";
    FE::String f;

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
    EXPECT_NE(a, d);
    EXPECT_EQ(e, f);
    EXPECT_NE(a, e);
    EXPECT_NE(d, f);
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
    EXPECT_EQ(FE::String{}.Compare(FE::String{}), 0);

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
    auto str   = FE::String("abc def 123");
    auto split = str.Split();
    ASSERT_EQ(split.Size(), 3);
    EXPECT_EQ(split[0], FE::StringSlice("abc"));
    EXPECT_EQ(split[1], FE::StringSlice("def"));
    EXPECT_EQ(split[2], FE::StringSlice("123"));
}

TEST(Strings, SplitSpace)
{
    auto str   = FE::String(" ");
    auto split = str.Split();
    ASSERT_EQ(split.Size(), 1);
    EXPECT_EQ(split[0], FE::StringSlice{});
}

TEST(Strings, SplitEmpty)
{
    auto str   = FE::String("");
    auto split = str.Split();
    ASSERT_EQ(split.Size(), 0);
}

TEST(Strings, SplitLines)
{
    auto str   = FE::String("abc\ndef\r\n123");
    auto split = str.SplitLines();
    ASSERT_EQ(split.Size(), 3);
    EXPECT_EQ(split[0], FE::StringSlice("abc"));
    EXPECT_EQ(split[1], FE::StringSlice("def"));
    EXPECT_EQ(split[2], FE::StringSlice("123"));
}

TEST(Strings, Strip)
{
    EXPECT_EQ(FE::String("123").Strip(), FE::StringSlice("123"));
    EXPECT_EQ(FE::String("    123    ").StripRight(), FE::StringSlice("    123"));
    EXPECT_EQ(FE::String("    123    ").StripLeft(), FE::StringSlice("123    "));
    EXPECT_EQ(FE::String(" \t \n \r   ").StripLeft(), FE::StringSlice{});
}

TEST(Strings, StartsWith)
{
    EXPECT_TRUE(FE::String("").StartsWith(""));
    EXPECT_TRUE(FE::String("1234").StartsWith(""));
    EXPECT_TRUE(FE::String("1234").StartsWith("1"));
    EXPECT_TRUE(FE::String("1234").StartsWith("12"));
    EXPECT_TRUE(FE::String("1234").StartsWith("1234"));
    EXPECT_FALSE(FE::String("1234").StartsWith("21"));
    EXPECT_FALSE(FE::String("1234").StartsWith("12345"));
}

TEST(Strings, EndsWith)
{
    EXPECT_TRUE(FE::String("").EndsWith(""));
    EXPECT_TRUE(FE::String("1234").EndsWith(""));
    EXPECT_TRUE(FE::String("1234").EndsWith("4"));
    EXPECT_TRUE(FE::String("1234").EndsWith("34"));
    EXPECT_TRUE(FE::String("1234").EndsWith("1234"));
    EXPECT_FALSE(FE::String("1234").EndsWith("21"));
    EXPECT_FALSE(FE::String("1234").EndsWith("12345"));
}

TEST(Strings, TryConvertTo)
{
    int i;
    char c;
    unsigned u;
    float f;
    bool b;
    EXPECT_TRUE(FE::String("1").TryConvertTo<int>(i));
    EXPECT_EQ(i, 1);
    EXPECT_TRUE(FE::String("-2").TryConvertTo<int>(i));
    EXPECT_EQ(i, -2);
    EXPECT_TRUE(FE::String("3").TryConvertTo<unsigned>(u));
    EXPECT_EQ(u, 3);
    EXPECT_TRUE(FE::String("1.5").TryConvertTo<float>(f));
    EXPECT_EQ(f, 1.5f);
    EXPECT_TRUE(FE::String("-1.5").TryConvertTo<float>(f));
    EXPECT_EQ(f, -1.5f);
    EXPECT_TRUE(FE::String("false").TryConvertTo<bool>(b));
    EXPECT_EQ(b, false);
    EXPECT_TRUE(FE::String("true").TryConvertTo<bool>(b));
    EXPECT_EQ(b, true);
    EXPECT_FALSE(FE::String("").TryConvertTo<int>(i));
    EXPECT_FALSE(FE::String("").TryConvertTo<bool>(b));
    EXPECT_FALSE(FE::String("-").TryConvertTo<int>(i));
    EXPECT_FALSE(FE::String("--123").TryConvertTo<int>(i));
    EXPECT_FALSE(FE::String("300").TryConvertTo<char>(c));
    EXPECT_FALSE(FE::String("123123123123123").TryConvertTo<int>(i));
    EXPECT_FALSE(FE::String("123qq").TryConvertTo<int>(i));
    EXPECT_FALSE(FE::String("qq123").TryConvertTo<int>(i));
    EXPECT_FALSE(FE::String("-123").TryConvertTo<unsigned>(u));
    EXPECT_FALSE(FE::String("--123").TryConvertTo<float>(f));
    EXPECT_FALSE(FE::String("123..7").TryConvertTo<float>(f));
    EXPECT_FALSE(FE::String("qq").TryConvertTo<bool>(b));
}

TEST(Strings, ConvertTo)
{
    EXPECT_EQ(FE::String("123").ConvertTo<FE::Int8>(), 123);
    EXPECT_EQ(FE::String("123").ConvertTo<FE::Int16>(), 123);
    EXPECT_EQ(FE::String("123").ConvertTo<FE::Int32>(), 123);
    EXPECT_EQ(FE::String("123").ConvertTo<FE::Int64>(), 123);

    EXPECT_EQ(FE::String("-123").ConvertTo<FE::Int8>(), -123);
    EXPECT_EQ(FE::String("-123").ConvertTo<FE::Int16>(), -123);
    EXPECT_EQ(FE::String("-123").ConvertTo<FE::Int32>(), -123);
    EXPECT_EQ(FE::String("-123").ConvertTo<FE::Int64>(), -123);

    EXPECT_EQ(FE::String("123").ConvertTo<FE::UInt8>(), 123);
    EXPECT_EQ(FE::String("123").ConvertTo<FE::UInt16>(), 123);
    EXPECT_EQ(FE::String("123").ConvertTo<FE::UInt32>(), 123);
    EXPECT_EQ(FE::String("123").ConvertTo<FE::UInt64>(), 123);

    EXPECT_EQ(FE::String("1.5").ConvertTo<FE::Float32>(), 1.5f);
    EXPECT_EQ(FE::String("-1.5").ConvertTo<FE::Float32>(), -1.5f);

    EXPECT_EQ(FE::String("1.5").ConvertTo<FE::Float64>(), 1.5f);
    EXPECT_EQ(FE::String("-1.5").ConvertTo<FE::Float64>(), -1.5f);

    EXPECT_EQ(FE::String("false").ConvertTo<bool>(), false);
    EXPECT_EQ(FE::String("true").ConvertTo<bool>(), true);
    EXPECT_EQ(FE::String("0").ConvertTo<bool>(), false);
    EXPECT_EQ(FE::String("1").ConvertTo<bool>(), true);
}
