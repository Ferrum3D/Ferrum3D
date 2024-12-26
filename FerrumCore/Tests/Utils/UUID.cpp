#include <FeCore/Strings/StringSlice.h>
#include <FeCore/Utils/UUID.h>
#include <Tests/Common/TestCommon.h>

TEST(UUID, Parse)
{
    const FE::UUID uuid1("62e1b7a1-c14a-4129-ac57-7e77289123e9");
    const FE::UUID uuid2("62E1B7A1-C14A-4129-AC57-7E77289123E9");

    ASSERT_EQ(uuid1.m_bytes[0], 0x62);
    ASSERT_EQ(uuid1.m_bytes[1], 0xe1);
    ASSERT_EQ(uuid1.m_bytes[14], 0x23);
    ASSERT_EQ(uuid1.m_bytes[15], 0xe9);

    ASSERT_EQ(uuid2.m_bytes[0], 0x62);
    ASSERT_EQ(uuid2.m_bytes[1], 0xe1);
    ASSERT_EQ(uuid2.m_bytes[14], 0x23);
    ASSERT_EQ(uuid2.m_bytes[15], 0xe9);

    ASSERT_EQ(uuid1, uuid2);

    ASSERT_EQ(uuid1, FE::UUID("{62e1b7a1-c14a-4129-ac57-7e77289123e9}"));
    ASSERT_EQ(uuid1, FE::UUID("{62e1b7a1c14a4129ac577e77289123e9}"));
    ASSERT_EQ(uuid1, FE::UUID("62e1b7a1c14a4129ac577e77289123e9"));

    const auto uuid3 = FE::StringSlice("62E1B7A1-C14A-4129-AC57-7E77289123E9").Parse<FE::UUID>();
    ASSERT_EQ(uuid1, uuid3.value());

    EXPECT_TRUE(FE::UUID::Parse("62E1B7A1-C14A-4129-AC57-7E77289123E9").IsValid());
    EXPECT_TRUE(FE::UUID::Parse("62e1b7a1-c14a-4129-ac57-7e77289123e9").IsValid());
    EXPECT_TRUE(FE::UUID::Parse("62e1B7A1-C14A-4129-Ac57-7E77289123E9").IsValid());

    EXPECT_FALSE(FE::UUID::Parse("62E1B7A1C14A-4129-AC57-7E77289123E9").IsValid());

    // incorrect length
    EXPECT_FALSE(FE::UUID::Parse("62E1B7A1-C14A-4129-AC57-7E77289123E").IsValid());
    EXPECT_FALSE(FE::UUID::Parse("62E1B7A1-C14A-4129-AC57-7E77289123E99999999999").IsValid());

    EXPECT_FALSE(FE::UUID::Parse("").IsValid());
    EXPECT_FALSE(FE::UUID::Parse("something weird").IsValid());

    // G - invalid character in different positions
    EXPECT_FALSE(FE::UUID::Parse("62E1B7A1-C14A-4129-AC57-7E77289123EG").IsValid());
    EXPECT_FALSE(FE::UUID::Parse("G2E1B7A1-C14A-4129-AC57-7E77289123E9").IsValid());
    EXPECT_FALSE(FE::UUID::Parse("6GE1B7A1-C14A-4129-AC57-7E77289123E9").IsValid());
    EXPECT_FALSE(FE::UUID::Parse("62G1B7A1-C14A-4129-AC57-7E77289123E9").IsValid());
    EXPECT_FALSE(FE::UUID::Parse("62EGB7A1-C14A-4129-AC57-7E77289123E9").IsValid());
    EXPECT_FALSE(FE::UUID::Parse("62E1G7A1-C14A-4129-AC57-7E77289123E9").IsValid());
}
