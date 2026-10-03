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


TEST(Aabb, UnionWithInteriorPointPreservesBounds)
{
    const Aabb bounds{ Vector3(-3.0f, -2.0f, -1.0f), Vector3(4.0f, 5.0f, 6.0f) };
    const Aabb result = Math::Union(bounds, Vector3(1.0f, 2.0f, 3.0f));

    EXPECT_EQ(result.min, bounds.min);
    EXPECT_EQ(result.max, bounds.max);
    EXPECT_TRUE(result.IsValid());
}


TEST(Aabb, UnionWithPointExpandsOnlyRequiredAxes)
{
    const Aabb bounds{ Vector3(-3.0f, -2.0f, -1.0f), Vector3(4.0f, 5.0f, 6.0f) };
    const Aabb result = Math::Union(bounds, Vector3(-7.0f, 9.0f, 2.0f));

    EXPECT_EQ(result.min, Vector3(-7.0f, -2.0f, -1.0f));
    EXPECT_EQ(result.max, Vector3(4.0f, 9.0f, 6.0f));
    EXPECT_TRUE(Math::Contains(result, bounds));
    EXPECT_TRUE(Math::Contains(result, Aabb{ Vector3(-7.0f, 9.0f, 2.0f), Vector3(-7.0f, 9.0f, 2.0f) }));
}


TEST(Aabb, UnionWithInvalidBoundsStartsPointAccumulation)
{
    const Vector3 firstPoint(3.0f, -2.0f, 7.0f);
    Aabb bounds = Math::Union(Aabb::kInvalid, firstPoint);
    EXPECT_EQ(bounds.min, firstPoint);
    EXPECT_EQ(bounds.max, firstPoint);
    EXPECT_TRUE(bounds.IsValid());

    bounds = Math::Union(bounds, Vector3(-4.0f, 5.0f, 1.0f));
    bounds = Math::Union(bounds, Vector3(1.0f, 0.0f, 3.0f));
    bounds = Math::Union(bounds, Vector3(2.0f, -6.0f, 9.0f));
    EXPECT_EQ(bounds.min, Vector3(-4.0f, -6.0f, 1.0f));
    EXPECT_EQ(bounds.max, Vector3(3.0f, 5.0f, 9.0f));
}


TEST(Aabb, UnionOfBoxes)
{
    const Aabb lhs{ Vector3(-3.0f, -2.0f, -1.0f), Vector3(4.0f, 5.0f, 6.0f) };
    const Aabb rhs{ Vector3(2.0f, -7.0f, -8.0f), Vector3(9.0f, 3.0f, 2.0f) };
    const Aabb result = Math::Union(lhs, rhs);

    EXPECT_EQ(result.min, Vector3(-3.0f, -7.0f, -8.0f));
    EXPECT_EQ(result.max, Vector3(9.0f, 5.0f, 6.0f));
    EXPECT_TRUE(Math::Contains(result, lhs));
    EXPECT_TRUE(Math::Contains(result, rhs));
    EXPECT_TRUE(Math::CmpEqual(result, Math::Union(rhs, lhs)));
    EXPECT_TRUE(Math::CmpEqual(Math::Union(lhs, lhs), lhs));
    EXPECT_TRUE(Math::CmpEqual(Math::Union(lhs, Aabb::kInvalid), lhs));
    EXPECT_TRUE(Math::CmpEqual(Math::Union(Aabb::kInvalid, lhs), lhs));
}


TEST(Aabb, ValidityAndDegenerateBounds)
{
    EXPECT_TRUE(Aabb::kZero.IsValid());
    EXPECT_FALSE(Aabb::kInvalid.IsValid());
    EXPECT_TRUE((Aabb{ Vector3(1.0f, 2.0f, 3.0f), Vector3(1.0f, 2.0f, 3.0f) }).IsValid());
    EXPECT_FALSE((Aabb{ Vector3(2.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f) }).IsValid());
    EXPECT_FALSE((Aabb{ Vector3(0.0f, 2.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f) }).IsValid());
    EXPECT_FALSE((Aabb{ Vector3(0.0f, 0.0f, 2.0f), Vector3(1.0f, 1.0f, 1.0f) }).IsValid());
}


TEST(Aabb, BoundaryOverlapAndContainment)
{
    const Aabb bounds{ Vector3(-1.0f), Vector3(1.0f) };
    const Aabb touching[] = {
        { Vector3(1.0f, -1.0f, -1.0f), Vector3(2.0f, 1.0f, 1.0f) },
        { Vector3(1.0f, 1.0f, -1.0f), Vector3(2.0f, 2.0f, 1.0f) },
        { Vector3(1.0f), Vector3(2.0f) },
        { Vector3(-1.0f), Vector3(-1.0f) },
    };
    for (const Aabb& other : touching)
    {
        EXPECT_TRUE(Math::Overlaps(bounds, other));
        EXPECT_TRUE(Math::Overlaps(other, bounds));
    }

    EXPECT_TRUE(Math::Contains(bounds, touching[3]));
    EXPECT_FALSE(Math::Overlaps(bounds, Aabb::kInvalid));
    EXPECT_FALSE(Math::Overlaps(Aabb::kInvalid, bounds));
    EXPECT_FALSE(Math::Contains(Aabb::kInvalid, bounds));
    EXPECT_FALSE(Math::Contains(bounds, Aabb{ Vector3(-1.0f, -2.0f, -1.0f), Vector3(1.0f) }));
    EXPECT_FALSE(Math::Contains(bounds, Aabb{ Vector3(-1.0f), Vector3(1.0f, 2.0f, 1.0f) }));
    EXPECT_FALSE(Math::Contains(bounds, Aabb{ Vector3(-1.0f, -1.0f, -2.0f), Vector3(1.0f) }));
    EXPECT_FALSE(Math::Contains(bounds, Aabb{ Vector3(-1.0f), Vector3(1.0f, 1.0f, 2.0f) }));
}


TEST(Aabb, PointDistance)
{
    const Aabb bounds{ Vector3(-1.0f), Vector3(1.0f) };
    EXPECT_FLOAT_EQ(Math::DistanceSquared(bounds, Vector3::kZero), 0.0f);
    EXPECT_FLOAT_EQ(Math::DistanceSquared(bounds, Vector3(1.0f)), 0.0f);
    EXPECT_FLOAT_EQ(Math::DistanceSquared(bounds, Vector3(3.0f, 0.0f, 0.0f)), 4.0f);
    EXPECT_FLOAT_EQ(Math::DistanceSquared(bounds, Vector3(3.0f, 4.0f, 0.0f)), 13.0f);
    EXPECT_FLOAT_EQ(Math::DistanceSquared(bounds, Vector3(-3.0f, 4.0f, 7.0f)), 49.0f);
    EXPECT_FLOAT_EQ(Math::Distance(bounds, Vector3(-3.0f, 4.0f, 7.0f)), 7.0f);
}


TEST(Aabb, OffsetAndPackedRoundTrip)
{
    const Aabb bounds{ Vector3(-3.0f, -2.0f, -1.0f), Vector3(4.0f, 5.0f, 6.0f) };
    const Aabb offset = Math::Offset(bounds, Vector3(7.0f, -8.0f, 9.0f));
    EXPECT_EQ(offset.min, Vector3(4.0f, -10.0f, 8.0f));
    EXPECT_EQ(offset.max, Vector3(11.0f, -3.0f, 15.0f));
    EXPECT_EQ(offset.Size(), bounds.Size());
    EXPECT_TRUE(Math::CmpEqual(static_cast<Aabb>(PackedAabb(bounds)), bounds));
}


TEST(Aabb, IgnoresSimdPadding)
{
    const Aabb bounds{ _mm_setr_ps(-3.0f, -2.0f, -1.0f, 100.0f), _mm_setr_ps(4.0f, 5.0f, 6.0f, -100.0f) };
    const Aabb expected{ Vector3(-3.0f, -2.0f, -1.0f), Vector3(4.0f, 5.0f, 6.0f) };
    const Vector3 point(_mm_setr_ps(1.0f, 2.0f, 3.0f, 200.0f));
    EXPECT_TRUE(bounds.IsValid());
    EXPECT_TRUE(Math::Contains(bounds, expected));
    EXPECT_TRUE(Math::Overlaps(bounds, expected));
    EXPECT_TRUE(Math::CmpEqual(bounds, expected));
    EXPECT_TRUE(Math::CmpEqual(Math::Union(bounds, point), expected));
}
