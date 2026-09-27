#include <Core/Math/Aabb.h>
#include <gtest/gtest.h>

using namespace FE;

TEST(Aabb, Overlaps)
{
    const Aabb bounds{ Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f) };

    EXPECT_TRUE(Math::Overlaps(bounds, bounds));
    EXPECT_TRUE(Math::Overlaps(bounds, Aabb{ Vector3(1.0f, 0.0f, 0.0f), Vector3(2.0f, 1.0f, 1.0f) }));
    EXPECT_FALSE(Math::Overlaps(bounds, Aabb{ Vector3(2.0f, 0.0f, 0.0f), Vector3(3.0f, 1.0f, 1.0f) }));
    EXPECT_FALSE(Math::Overlaps(bounds, Aabb{ Vector3(0.0f, 2.0f, 0.0f), Vector3(1.0f, 3.0f, 1.0f) }));
    EXPECT_FALSE(Math::Overlaps(bounds, Aabb{ Vector3(0.0f, 0.0f, 2.0f), Vector3(1.0f, 1.0f, 3.0f) }));
}


TEST(Aabb, Contains)
{
    const Aabb bounds{ Vector3(-2.0f, -2.0f, -2.0f), Vector3(2.0f, 2.0f, 2.0f) };

    EXPECT_TRUE(Math::Contains(bounds, bounds));
    EXPECT_TRUE(Math::Contains(bounds, Aabb{ Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f) }));
    EXPECT_FALSE(Math::Contains(bounds, Aabb{ Vector3(-3.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f) }));
    EXPECT_FALSE(Math::Contains(bounds, Aabb{ Vector3(-1.0f, -1.0f, -1.0f), Vector3(3.0f, 1.0f, 1.0f) }));
}
