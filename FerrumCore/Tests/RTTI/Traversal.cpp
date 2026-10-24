#include <Core/IO/AssetReferences.h>
#include <Core/RTTI/ReflectionContext.h>
#include <Core/RTTI/Traversal.h>
#include <RTTI/TraversalTypes.h>
#include <gtest/gtest.h>

using namespace FE;
using namespace FE::Rtti::Tests;

namespace
{
    const IO::AssetID kFirst("A1000000-0000-4000-8000-000000000001");
    const IO::AssetID kSecond("A1000000-0000-4000-8000-000000000002");
    const IO::AssetID kThird("A1000000-0000-4000-8000-000000000003");
    const IO::AssetID kFourth("A1000000-0000-4000-8000-000000000004");
    const IO::AssetID kFifth("A1000000-0000-4000-8000-000000000005");
    const IO::AssetID kRuntime("A1000000-0000-4000-8000-000000000006");


    TraversalLeaf MakeLeaf(const int32_t value, const IO::AssetID asset = IO::AssetID::kNull)
    {
        TraversalLeaf leaf;
        leaf.m_value = value;
        leaf.m_link.SetAssetID(asset);
        return leaf;
    }


    //! A root with references in every kind of container.
    TraversalRoot MakeRoot()
    {
        TraversalRoot root;
        root.m_leaves.push_back(MakeLeaf(1, kFirst));
        root.m_leaves.push_back(MakeLeaf(2));
        root.m_fixed[1] = MakeLeaf(3, kSecond);
        root.m_choice = MakeLeaf(4, kThird);

        auto* object = Memory::DefaultNew<TraversalDerivedObject>();
        object->m_baseValue = 5;
        object->m_leaves.push_back(MakeLeaf(6, kFourth));
        root.m_object = object;

        root.m_any.Emplace<TraversalLeaf>(MakeLeaf(7, kFifth));
        root.m_numbers = { 8, 9 };
        root.m_name = "root";
        root.m_offset = Vector3(1, 2, 3);
        root.m_runtimeLink.SetAssetID(kRuntime);
        root.m_runtimeValue = 10;
        return root;
    }


    const Rtti::FieldInfo& FindField(const Rtti::Type& type, const festd::ascii_view name)
    {
        for (const Rtti::FieldInfo& field : type.m_fields)
        {
            if (field.m_name == name)
                return field;
        }

        FE_DebugBreak();
        return type.m_fields[0];
    }
} // namespace


TEST(RttiShapes, FieldTypesDescribeTheirStructure)
{
    const Rtti::Type& type = Rtti::GetType<TraversalRoot>();
    EXPECT_EQ(type.m_shape, Rtti::TypeShape::kStruct);

    const auto* leaves = FindField(type, "m_leaves").m_type->TryGetShape<Rtti::SequenceShape>();
    ASSERT_NE(leaves, nullptr);
    EXPECT_EQ(leaves->m_element, &Rtti::GetType<TraversalLeaf>());

    const auto* fixed = FindField(type, "m_fixed").m_type->TryGetShape<Rtti::FixedArrayShape>();
    ASSERT_NE(fixed, nullptr);
    EXPECT_EQ(fixed->m_count, 2u);
    EXPECT_EQ(fixed->m_stride, sizeof(TraversalLeaf));

    const auto* choice = FindField(type, "m_choice").m_type->TryGetShape<Rtti::VariantShape>();
    ASSERT_NE(choice, nullptr);
    ASSERT_EQ(choice->m_alternatives.size(), 2u);
    EXPECT_EQ(choice->m_alternatives[0], &Rtti::GetType<int32_t>());
    EXPECT_EQ(choice->m_alternatives[1], &Rtti::GetType<TraversalLeaf>());

    const auto* object = FindField(type, "m_object").m_type->TryGetShape<Rtti::DynamicShape>();
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->m_base, &Rtti::GetType<TraversalObject>());
    EXPECT_EQ(FindField(type, "m_any").m_type->m_shape, Rtti::TypeShape::kDynamic);
    EXPECT_EQ(FindField(type, "m_numbers").m_type->m_shape, Rtti::TypeShape::kSequence);
    EXPECT_EQ(FindField(type, "m_name").m_type->m_shape, Rtti::TypeShape::kString);

    const Rtti::Type& link = *FindField(type, "m_runtimeLink").m_type;
    ASSERT_EQ(link.m_shape, Rtti::TypeShape::kHandle);
    const auto* reference = link.TryGetAttribute<IO::AssetReference>();
    ASSERT_NE(reference, nullptr);
    EXPECT_EQ(reference->m_getAssetType(), Rtti::GetTypeID<TraversalLeaf>());
    EXPECT_EQ(reference->m_kind, IO::DependencyKind::kHard);
}


TEST(RttiShapes, ContainmentFollowsStaticStructure)
{
    EXPECT_TRUE(Rtti::CanContainShape(Rtti::GetType<TraversalRoot>(), Rtti::TypeShape::kHandle));
    EXPECT_TRUE(Rtti::CanContainShape(Rtti::ResolveType<festd::vector<TraversalLeaf>>(), Rtti::TypeShape::kHandle));
    EXPECT_FALSE(Rtti::CanContainShape(Rtti::ResolveType<festd::vector<uint32_t>>(), Rtti::TypeShape::kHandle));
    EXPECT_TRUE(Rtti::CanContainShape(Rtti::GetType<Rtti::Any>(), Rtti::TypeShape::kHandle));
}


TEST(RttiTraversal, AssetReferencesAreFoundInContainersAndDynamicValues)
{
    const TraversalRoot root = MakeRoot();
    festd::vector<IO::AssetID> found;
    IO::VisitAssetReferences(Rtti::GetType<TraversalRoot>(), &root, [&](const IO::AssetReferenceInfo& reference) {
        EXPECT_EQ(reference.m_assetType, Rtti::GetTypeID<TraversalLeaf>());
        EXPECT_EQ(reference.m_kind, IO::DependencyKind::kHard);
        found.push_back(reference.m_assetId);
    });

    // Runtime-only fields are skipped like they are by serialization.
    const festd::vector<IO::AssetID> expected{ kFirst, kSecond, kThird, kFourth, kFifth };
    EXPECT_EQ(found, expected);
}


TEST(RttiTraversal, PathsResolveToTheVisitedValues)
{
    TraversalRoot root = MakeRoot();
    const Rtti::Type& type = Rtti::GetType<TraversalRoot>();

    festd::vector<Rtti::ValuePath> leafPaths;
    Rtti::Visit(type, &root, Rtti::VisitFlags::kNone, [&](const Rtti::VisitNode& node) {
        if (&node.m_type == &Rtti::GetType<TraversalLeaf>())
            leafPaths.push_back(node.m_path);

        return Rtti::VisitResult::kContinue;
    });

    // Two sequence elements, two fixed elements, the variant, the object's sequence and the Any.
    ASSERT_EQ(leafPaths.size(), 7u);

    festd::vector<int32_t> values;
    for (const Rtti::ValuePath& path : leafPaths)
    {
        const Rtti::ValueRef leaf = Rtti::ResolvePath(type, &root, path);
        ASSERT_TRUE(leaf);
        ASSERT_EQ(leaf.m_type, &Rtti::GetType<TraversalLeaf>());
        values.push_back(static_cast<const TraversalLeaf*>(leaf.m_value)->m_value);
    }

    const festd::vector<int32_t> expected{ 1, 2, 0, 3, 4, 6, 7 };
    EXPECT_EQ(values, expected);

    // A path into the variant's alternative stops resolving once the alternative changes.
    const Rtti::ValuePath variantPath = leafPaths[4];
    root.m_choice = 11;
    EXPECT_FALSE(Rtti::ResolvePath(type, &root, variantPath));
}


TEST(RttiTraversal, VisitCanSkipChildrenAndStop)
{
    TraversalRoot root = MakeRoot();
    const Rtti::Type& type = Rtti::GetType<TraversalRoot>();

    uint32_t leaves = 0;
    Rtti::Visit(type, &root, Rtti::VisitFlags::kNone, [&](const Rtti::VisitNode& node) {
        if (node.m_type.m_shape == Rtti::TypeShape::kDynamic)
            return Rtti::VisitResult::kSkipChildren;

        leaves += &node.m_type == &Rtti::GetType<TraversalLeaf>() ? 1 : 0;
        return Rtti::VisitResult::kContinue;
    });
    EXPECT_EQ(leaves, 5u);

    uint32_t visited = 0;
    const bool completed = Rtti::Visit(type, &root, Rtti::VisitFlags::kNone, [&](const Rtti::VisitNode&) {
        return ++visited == 3 ? Rtti::VisitResult::kStop : Rtti::VisitResult::kContinue;
    });
    EXPECT_FALSE(completed);
    EXPECT_EQ(visited, 3u);
}


TEST(RttiTraversal, CopyKeepsDestinationRuntimeStateAndDeepCopiesObjects)
{
    const TraversalRoot source = MakeRoot();
    TraversalRoot destination;
    destination.m_runtimeValue = 99;
    destination.m_numbers = { 1, 2, 3, 4 };

    ASSERT_TRUE(Rtti::CopyValue(Rtti::GetType<TraversalRoot>(), &source, &destination, Rtti::VisitFlags::kSkipTransient));
    EXPECT_EQ(destination.m_runtimeValue, 99);
    EXPECT_FALSE(destination.m_runtimeLink.GetAssetID().IsValid());

    ASSERT_EQ(destination.m_leaves.size(), 2u);
    EXPECT_EQ(destination.m_leaves[0].m_link.GetAssetID(), kFirst);
    EXPECT_EQ(destination.m_fixed[1].m_value, 3);
    ASSERT_EQ(destination.m_choice.index(), 1u);
    EXPECT_EQ(festd::get<TraversalLeaf>(destination.m_choice).m_link.GetAssetID(), kThird);
    EXPECT_EQ(destination.m_numbers.size(), 2u);
    EXPECT_EQ(destination.m_name, "root");

    // Math types keep their state in unions behind manual serializers, so they are copied whole.
    EXPECT_TRUE(Bit::AnySet(Rtti::GetType<Vector3>().m_flags, Rtti::TypeFlags::kHasUnreflectedFields));
    EXPECT_EQ(destination.m_offset, Vector3(1, 2, 3));

    // Objects are copied, not shared, like a serialization round trip.
    ASSERT_TRUE(destination.m_object);
    EXPECT_NE(destination.m_object.Get(), source.m_object.Get());
    const auto* object = Rtti::Cast<const TraversalDerivedObject*>(destination.m_object.Get());
    ASSERT_NE(object, nullptr);
    EXPECT_EQ(object->m_baseValue, 5);
    ASSERT_EQ(object->m_leaves.size(), 1u);
    EXPECT_EQ(object->m_leaves[0].m_link.GetAssetID(), kFourth);

    const auto* any = destination.m_any.TryGet<TraversalLeaf>();
    ASSERT_NE(any, nullptr);
    EXPECT_EQ(any->m_value, 7);
}
