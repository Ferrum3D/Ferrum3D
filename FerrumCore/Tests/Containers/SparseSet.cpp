#include <FeCore/Containers/SparseSet.h>
#include <FeCore/Strings/String.h>
#include <gtest/gtest.h>

using FE::SparseSet;

TEST(SparseSet, Create)
{
    SparseSet<FE::UInt32, FE::String> sparseSet(10);
    EXPECT_EQ(sparseSet.Capacity(), 10);
    EXPECT_EQ(sparseSet.Size(), 0);
}

TEST(SparseSet, Insert)
{
    SparseSet<FE::UInt32, FE::String> sparseSet(10);

    sparseSet.Insert(0, "Zero");
    EXPECT_EQ(sparseSet.Capacity(), 10);
    EXPECT_EQ(sparseSet.Size(), 1);

    sparseSet.Insert(9, "Nine");
    EXPECT_EQ(sparseSet.Capacity(), 10);
    EXPECT_EQ(sparseSet.Size(), 2);

    sparseSet.Emplace(9, "*", 5);
    EXPECT_EQ(sparseSet.Capacity(), 10);
    EXPECT_EQ(sparseSet.Size(), 2);
}

TEST(SparseSet, Contains)
{
    SparseSet<FE::UInt32, FE::String> sparseSet(10);
    sparseSet.Insert(0, "Zero");
    EXPECT_TRUE(sparseSet.Contains(0));
    sparseSet.Insert(9, "Nine");
    EXPECT_TRUE(sparseSet.Contains(0));
    EXPECT_TRUE(sparseSet.Contains(9));
}

TEST(SparseSet, TryGetAt)
{
    SparseSet<FE::UInt32, FE::String> sparseSet(10);
    sparseSet.Insert(0, "Zero");
    FE::String value;
    EXPECT_TRUE(sparseSet.TryGetAt(0, value));
    EXPECT_EQ(value, "Zero");
    EXPECT_FALSE(sparseSet.TryGetAt(2, value));
}

TEST(SparseSet, Indexer)
{
    SparseSet<FE::UInt32, FE::String> sparseSet(10);
    sparseSet.Insert(0, "Zero");
    EXPECT_EQ(sparseSet[0], "Zero");

    sparseSet[0] = "ZeroZero";
    EXPECT_EQ(sparseSet[0], "ZeroZero");
}

TEST(SparseSet, Remove)
{
    SparseSet<FE::UInt32, FE::String> sparseSet(10);
    sparseSet.Insert(0, "Zero");
    sparseSet.Insert(1, "One");
    sparseSet.Insert(2, "Two");
    sparseSet.Remove(1);

    EXPECT_TRUE(sparseSet.Contains(0));
    EXPECT_FALSE(sparseSet.Contains(1));
    EXPECT_TRUE(sparseSet.Contains(2));

    EXPECT_EQ(sparseSet[0], "Zero");
    EXPECT_EQ(sparseSet[2], "Two");
}
