#include <Core/RTTI/ReflectionContext.h>
#include <gtest/gtest.h>

namespace
{
    struct Context final : FE::Rtti::ReflectionContext
    {
        void RegisterType(FE::Rtti::Type&) override {}
    };


    struct Movable
    {
        static inline int s_live = 0;
        int m_value = 17;

        Movable()
        {
            ++s_live;
        }


        Movable(const Movable&) = delete;
        Movable(Movable&& other)
            : m_value(std::exchange(other.m_value, -1))
        {
            ++s_live;
        }


        ~Movable()
        {
            --s_live;
        }
    };


    struct Immovable
    {
        Immovable() = default;
        Immovable(const Immovable&) = delete;
        Immovable(Immovable&&) = delete;
    };
} // namespace

TEST(RTTIMove, TypeErasedMoveDestroysBothObjectsExactlyOnce)
{
    Context context;
    FE::Rtti::Type type;
    context.ReflectBuiltinType<Movable>(type, FE::Uuid::kNull, "Movable");
    ASSERT_NE(type.m_moveConstructor, nullptr);
    ASSERT_EQ(type.m_copyConstructor, nullptr);

    alignas(Movable) std::byte source[sizeof(Movable)], destination[sizeof(Movable)];
    type.m_defaultConstructor(source);
    type.m_moveConstructor(destination, source);
    EXPECT_EQ(reinterpret_cast<Movable*>(destination)->m_value, 17);
    EXPECT_EQ(reinterpret_cast<Movable*>(source)->m_value, -1);
    EXPECT_EQ(Movable::s_live, 2);
    type.m_destructor(source);
    type.m_destructor(destination);
    EXPECT_EQ(Movable::s_live, 0);
}


TEST(RTTIMove, UnsupportedMoveHasNoConstructor)
{
    Context context;
    FE::Rtti::Type type;
    context.ReflectBuiltinType<Immovable>(type, FE::Uuid::kNull, "Immovable");
    EXPECT_EQ(type.m_moveConstructor, nullptr);
}
