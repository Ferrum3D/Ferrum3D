#include <FeCore/Containers/List.h>
#include <gtest/gtest.h>
#include <Tests/Common/TestCommon.h>

using FE::List;

TEST(List, Construct)
{
    auto mock = std::make_shared<MockConstructors>();
    EXPECT_CALL(*mock, Construct()).Times(1);
    EXPECT_CALL(*mock, Destruct()).Times(1);
    EXPECT_CALL(*mock, Copy()).Times(0);
    EXPECT_CALL(*mock, Move()).Times(0);

    List<MockConstructors> lst;
    lst.Emplace();
}
