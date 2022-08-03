#include <FeCore/Strings/Format.h>
#include <gtest/gtest.h>

namespace Fmt = FE::Fmt;

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

TEST(Format, UUID)
{
    FE::UUID uuid("62e1b7a1-c14a-4129-ac57-7e77289123e9");
    EXPECT_EQ(Fmt::Format("{}", uuid), "62E1B7A1-C14A-4129-AC57-7E77289123E9");
    EXPECT_EQ(Fmt::Format("Asset loader for `{}` not found", uuid), "Asset loader for `62E1B7A1-C14A-4129-AC57-7E77289123E9` not found");
}

TEST(Format, Strings)
{
    // EXPECT_EQ(Fmt::Format("{}", "literal"), "literal");
    EXPECT_EQ(Fmt::Format("{}", FE::StringSlice("slice")), "slice");
    EXPECT_EQ(Fmt::Format("{}", FE::String("str")), FE::String("str"));
    EXPECT_EQ(
        Fmt::Format("{}", FE::String("loooooooooooooooooooooooooooooong str")),
        FE::String("loooooooooooooooooooooooooooooong str"));
}
