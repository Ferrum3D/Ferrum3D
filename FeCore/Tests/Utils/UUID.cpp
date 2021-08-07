#include <FeCore/Utils/UUID.h>
#include <gtest/gtest.h>

TEST(UUID, Parse)
{
    FE::UUID uuid1 = "62e1b7a1-c14a-4129-ac57-7e77289123e9";
    FE::UUID uuid2 = "62E1B7A1-C14A-4129-AC57-7E77289123E9";

    ASSERT_EQ(uuid1.Data[0], 0x62);
    ASSERT_EQ(uuid1.Data[1], 0xe1);
    ASSERT_EQ(uuid1.Data[14], 0x23);
    ASSERT_EQ(uuid1.Data[15], 0xe9);

    ASSERT_EQ(uuid2.Data[0], 0x62);
    ASSERT_EQ(uuid2.Data[1], 0xe1);
    ASSERT_EQ(uuid2.Data[14], 0x23);
    ASSERT_EQ(uuid2.Data[15], 0xe9);

    ASSERT_EQ(uuid1, uuid2);
}
