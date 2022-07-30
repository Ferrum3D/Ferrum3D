#include <FeCore/Strings/StringSlice.h>
#include <FeCore/Utils/UUID.h>
#include <gtest/gtest.h>

TEST(UUID, Parse)
{
    FE::UUID uuid1("62e1b7a1-c14a-4129-ac57-7e77289123e9");
    FE::UUID uuid2("62E1B7A1-C14A-4129-AC57-7E77289123E9");

    ASSERT_EQ(uuid1.Data[0], 0x62);
    ASSERT_EQ(uuid1.Data[1], 0xe1);
    ASSERT_EQ(uuid1.Data[14], 0x23);
    ASSERT_EQ(uuid1.Data[15], 0xe9);

    ASSERT_EQ(uuid2.Data[0], 0x62);
    ASSERT_EQ(uuid2.Data[1], 0xe1);
    ASSERT_EQ(uuid2.Data[14], 0x23);
    ASSERT_EQ(uuid2.Data[15], 0xe9);

    ASSERT_EQ(uuid1, uuid2);

    auto uuid3 = FE::StringSlice("62E1B7A1-C14A-4129-AC57-7E77289123E9").ConvertTo<FE::UUID>();
    ASSERT_EQ(uuid1, uuid3);
}

TEST(UUID, TryParse)
{
    FE::UUID result;
    EXPECT_TRUE(FE::UUID::TryParse("62E1B7A1-C14A-4129-AC57-7E77289123E9", result));
    EXPECT_TRUE(FE::UUID::TryParse("62e1b7a1-c14a-4129-ac57-7e77289123e9", result));
    EXPECT_TRUE(FE::UUID::TryParse("62e1B7A1-C14A-4129-Ac57-7E77289123E9", result));

    EXPECT_FALSE(FE::UUID::TryParse("62E1B7A1C14A-4129-AC57-7E77289123E9", result));
    EXPECT_FALSE(FE::UUID::TryParse("62E1B7A1=C14A-4129-AC57-7E77289123E9", result));

    // incorrect length
    EXPECT_FALSE(FE::UUID::TryParse("62E1B7A1-C14A-4129-AC57-7E77289123E", result));
    EXPECT_FALSE(FE::UUID::TryParse("62E1B7A1-C14A-4129-AC57-7E77289123E99999999999", result));
    EXPECT_TRUE(FE::UUID::TryParse("62E1B7A1-C14A-4129-AC57-7E77289123E99999999999", result, false));

    EXPECT_FALSE(FE::UUID::TryParse("", result));
    EXPECT_FALSE(FE::UUID::TryParse("something weird", result));

    // G - invalid character in different positions
    EXPECT_FALSE(FE::UUID::TryParse("62E1B7A1-C14A-4129-AC57-7E77289123EG", result));
    EXPECT_FALSE(FE::UUID::TryParse("G2E1B7A1-C14A-4129-AC57-7E77289123E9", result));
    EXPECT_FALSE(FE::UUID::TryParse("6GE1B7A1-C14A-4129-AC57-7E77289123E9", result));
    EXPECT_FALSE(FE::UUID::TryParse("62G1B7A1-C14A-4129-AC57-7E77289123E9", result));
    EXPECT_FALSE(FE::UUID::TryParse("62EGB7A1-C14A-4129-AC57-7E77289123E9", result));
    EXPECT_FALSE(FE::UUID::TryParse("62E1G7A1-C14A-4129-AC57-7E77289123E9", result));
}
