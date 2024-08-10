#include <FeCore/Containers/SegmentedVector.h>
#include <gtest/gtest.h>

using namespace FE;

TEST(SegmentedVector, PushBack)
{
    SegmentedVector<int32_t, 8> vec;
    EXPECT_TRUE(vec.empty());

    vec.push_back();
    vec.push_back() = 1;
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);

    festd::vector<int32_t> temp;
    for (int32_t value : vec)
    {
        temp.push_back(value);
    }

    EXPECT_EQ(temp.size(), 5);
    EXPECT_EQ(vec.size(), 5);
    EXPECT_FALSE(vec.empty());

    for (int32_t valueIndex = 0; valueIndex < 5; ++valueIndex)
    {
        EXPECT_EQ(valueIndex, temp[valueIndex]);
        EXPECT_EQ(valueIndex, vec[valueIndex]);
    }
}

TEST(SegmentedVector, Clear)
{
    SegmentedVector<int32_t, 8> vec;
    vec.push_back(0);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);
    EXPECT_EQ(vec.size(), 5);
    EXPECT_FALSE(vec.empty());

    int32_t** ppSegments = vec.segments();
    int32_t* pFirstSegment = ppSegments[0];

    vec.clear();
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(vec.empty());
    EXPECT_TRUE(ppSegments == vec.segments());
    EXPECT_TRUE(ppSegments[0] == pFirstSegment);

    vec.push_back(0);
    vec.push_back(1);
    vec.push_back(2);
    EXPECT_EQ(vec.size(), 3);
    EXPECT_FALSE(vec.empty());
    EXPECT_TRUE(ppSegments == vec.segments());
    EXPECT_TRUE(ppSegments[0] == pFirstSegment);
}

TEST(SegmentedVector, CopyMove)
{
    SegmentedVector<int32_t, 8> vec;
    SegmentedVector<int32_t, 8> empty = vec;

    vec.push_back(0);
    SegmentedVector<int32_t, 8> one = vec;

    vec.push_back(1);
    SegmentedVector<int32_t, 8> two(vec);

    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);
    SegmentedVector<int32_t, 16> many = vec;

    EXPECT_TRUE(empty.empty());
    EXPECT_NE(empty.segments(), vec.segments());
    empty.push_back(123);
    EXPECT_NE(empty.segments()[0], vec.segments()[0]);
    EXPECT_EQ(empty[0], 123);
    EXPECT_EQ(empty.size(), 1);

    EXPECT_EQ(one.size(), 1);
    EXPECT_NE(one.segments(), vec.segments());
    EXPECT_NE(one.segments()[0], vec.segments()[0]);
    EXPECT_EQ(two.size(), 2);
    EXPECT_NE(two.segments(), vec.segments());
    EXPECT_NE(two.segments()[0], vec.segments()[0]);
    EXPECT_EQ(many.size(), 5);
    EXPECT_NE(many.segments(), vec.segments());
    EXPECT_NE(many.segments()[0], vec.segments()[0]);
    EXPECT_EQ(vec.size(), 5);

    for (int32_t valueIndex = 0; valueIndex < 5; ++valueIndex)
    {
        EXPECT_EQ(valueIndex, vec[valueIndex]);
        EXPECT_EQ(valueIndex, many[valueIndex]);
    }

    int32_t** ppSegments = vec.segments();
    int32_t* pFirstSegment = ppSegments[0];
    SegmentedVector<int32_t, 8> moved = std::move(vec);
    EXPECT_EQ(ppSegments, moved.segments());
    EXPECT_EQ(pFirstSegment, moved.segments()[0]);

    for (int32_t valueIndex = 0; valueIndex < 5; ++valueIndex)
    {
        EXPECT_EQ(valueIndex, moved[valueIndex]);
        EXPECT_EQ(valueIndex, many[valueIndex]);
    }
}

TEST(SegmentedVector, Iterator)
{
    SegmentedVector<int32_t, 8> vec;
    vec.push_back(0);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);

    auto iter = vec.begin() + 2;
    EXPECT_EQ(*iter, 2);
    EXPECT_EQ(*(iter - 2), 0);
    EXPECT_EQ(*(iter - 1), 1);
    EXPECT_EQ(*(iter + 1), 3);
    EXPECT_EQ(*(iter + 2), 4);

    --iter;
    EXPECT_EQ(*iter, 1);
    EXPECT_EQ(*(iter--), 1);
    EXPECT_EQ(*iter, 0);
    EXPECT_EQ(iter, vec.begin());

    iter += vec.size();
    EXPECT_EQ(iter, vec.end());
    EXPECT_EQ(iter - vec.size(), vec.begin());
    EXPECT_EQ(vec.end() - vec.size(), vec.begin());

    iter -= 2;
    EXPECT_EQ(*iter, 3);
}
