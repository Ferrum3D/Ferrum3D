#include <FeCore/Strings/Format.h>
#include <FeCore/Math/UUID.h>
#include <gtest/gtest.h>

using namespace FE;

TEST(Format, Escape)
{
    EXPECT_EQ(Fmt::Format("qqq{{"), "qqq{");
    EXPECT_EQ(Fmt::Format("{{qqq"), "{qqq");
    EXPECT_EQ(Fmt::Format("{{"), "{");
    EXPECT_EQ(Fmt::Format("qqq{{qqq"), "qqq{qqq");

    EXPECT_EQ(Fmt::Format("qqq}}"), "qqq}");
    EXPECT_EQ(Fmt::Format("}}qqq"), "}qqq");
    EXPECT_EQ(Fmt::Format("}}"), "}");
    EXPECT_EQ(Fmt::Format("qqq}}qqq"), "qqq}qqq");

    EXPECT_EQ(Fmt::Format("{{}}"), "{}");
    EXPECT_EQ(Fmt::Format("}}{{"), "}{");
    EXPECT_EQ(Fmt::Format("}}}}"), "}}");
    EXPECT_EQ(Fmt::Format("{{{{"), "{{");

    EXPECT_EQ(Fmt::Format("{{{}}}", 0), "{0}");
}

TEST(Format, Fixed)
{
    EXPECT_EQ(Fmt::FixedFormat("{} + {} = {}", 2, 2, 5), "2 + 2 = 5");
}

TEST(Format, ManualIndices)
{
    EXPECT_EQ(Fmt::Format("{1} {0} {0} {2}", 0, 1, 2), "1 0 0 2");
}

TEST(Format, UUID)
{
    const Uuid uuid("62e1b7a1-c14a-4129-ac57-7e77289123e9");
    EXPECT_EQ(Fmt::Format("{}", uuid), "62E1B7A1-C14A-4129-AC57-7E77289123E9");
    EXPECT_EQ(Fmt::Format("Asset loader for `{}` not found", uuid),
              "Asset loader for `62E1B7A1-C14A-4129-AC57-7E77289123E9` not found");
}

TEST(Format, Strings)
{
    const char* constString = "const char pointer";
    char mutableString[] = "mutable char array";

    EXPECT_EQ(Fmt::Format("{}", "literal"), "literal");
    EXPECT_EQ(Fmt::Format("{}", constString), "const char pointer");
    EXPECT_EQ(Fmt::Format("{}", mutableString), "mutable char array");
    EXPECT_EQ(Fmt::Format("{}", static_cast<char*>(mutableString)), "mutable char array");
    EXPECT_EQ(Fmt::Format("{}", festd::string_view("slice")), "slice");
    EXPECT_EQ(Fmt::Format("{}", festd::string("str")), festd::string("str"));
    EXPECT_EQ(Fmt::Format("{}", festd::string("loooooooooooooooooooooooooooooong str")),
              festd::string("loooooooooooooooooooooooooooooong str"));
}

TEST(Format, Pointers)
{
    int value = 0;
    const void* pointer = &value;
    EXPECT_EQ(Fmt::Format("{}", pointer), Fmt::Format("{}", reinterpret_cast<uintptr_t>(pointer)));
}
