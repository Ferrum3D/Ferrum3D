#include <Framework/Entities/EntityWorld.h>
#include <gtest/gtest.h>
#include <limits>

using namespace FE;
using namespace FE::Framework;

TEST(EntityFixedUpdate, FixedEpochsSelectFixedHooksAndDoNotTickFrameServices)
{
    struct Service final : WorldService
    {
        uint32_t m_updates = 0;
        void Update(EntityWorld&) override
        {
            ++m_updates;
        }
    } service;

    struct System final : WorldSystem
    {
        uint32_t m_updates = 0;
        uint32_t m_fixedUpdates = 0;
        uint32_t m_executed = 0;
        float m_delta = 0;
        void Update(EntityUpdateContext&) override
        {
            ++m_updates;
        }


        void FixedUpdate(EntityUpdateContext& context) override
        {
            ++m_fixedUpdates;
            m_delta = context.m_deltaSeconds;
            context.m_world.RecordStage(Phases::Update, [this] {
                ++m_executed;
            });
        }
    } system;

    EntityWorld world;
    world.AddService(service);
    world.AddSystem(system);
    for (uint32_t i = 0; i < 3; ++i)
    {
        world.BeginFixedUpdate(1.0f / 60.0f);
        ASSERT_TRUE(world.SchedulePhase(Phases::Update));
        ASSERT_TRUE(world.ExecuteSchedule());
        ASSERT_TRUE(world.EndUpdate());
    }

    EXPECT_EQ(system.m_updates, 0u);
    EXPECT_EQ(service.m_updates, 0u);
    EXPECT_EQ(system.m_fixedUpdates, 3u);
    EXPECT_EQ(system.m_executed, 3u);
    EXPECT_FLOAT_EQ(system.m_delta, 1.0f / 60.0f);

    world.BeginUpdate();
    ASSERT_TRUE(world.ExecuteSchedule());
    ASSERT_TRUE(world.EndUpdate());
    EXPECT_EQ(system.m_updates, 1u);
    EXPECT_EQ(service.m_updates, 1u);
    world.RemoveSystem(system);
    world.RemoveService(service);
}


TEST(EntityFixedUpdate, SharedPhasesOrderBothUpdateFrequencies)
{
    struct System final : WorldSystem
    {
        uint32_t m_order[6]{};
        uint32_t m_count = 0;

        void Update(EntityUpdateContext& context) override
        {
            Record(context, 10);
        }


        void FixedUpdate(EntityUpdateContext& context) override
        {
            Record(context, 20);
        }


        void Record(EntityUpdateContext& context, const uint32_t base)
        {
            context.m_world.RecordStage(Phases::PostUpdate, [this, base] {
                m_order[m_count++] = base + 3;
            });
            context.m_world.RecordStage(Phases::Update, [this, base] {
                m_order[m_count++] = base + 2;
            });
            context.m_world.RecordStage(Phases::PreUpdate, [this, base] {
                m_order[m_count++] = base + 1;
            });
        }
    } system;

    EntityWorld world;
    world.AddSystem(system);
    for (const bool fixed : { true, false })
    {
        if (fixed)
            world.BeginFixedUpdate(1.0f / 60.0f);
        else
            world.BeginUpdate();

        ASSERT_TRUE(world.SchedulePhase(Phases::PreUpdate));
        ASSERT_TRUE(world.SchedulePhase(Phases::Update));
        ASSERT_TRUE(world.SchedulePhase(Phases::PostUpdate));
        ASSERT_TRUE(world.ExecuteSchedule());
        ASSERT_TRUE(world.EndUpdate());
    }

    ASSERT_EQ(system.m_count, 6u);
    for (uint32_t i = 0; i < 3; ++i)
    {
        EXPECT_EQ(system.m_order[i], 21u + i);
        EXPECT_EQ(system.m_order[i + 3], 11u + i);
    }

    world.RemoveSystem(system);
}


TEST(EntityFixedUpdate, AccumulatesFractionsAndCapsCatchUp)
{
    EntityWorld world;
    uint32_t steps = 0;
    const auto schedule = [&steps](EntityWorld& fixedWorld) {
        fixedWorld.RecordStage(Phases::Update, [&steps] {
            ++steps;
        });
        EXPECT_TRUE(fixedWorld.SchedulePhase(Phases::Update));
    };

    ASSERT_TRUE(world.RunFixedUpdates(0.0625, schedule, 0.125f, 3));
    EXPECT_EQ(steps, 0u);
    ASSERT_TRUE(world.RunFixedUpdates(0.0625, schedule, 0.125f, 3));
    EXPECT_EQ(steps, 1u);
    ASSERT_TRUE(world.RunFixedUpdates(0.3125, schedule, 0.125f, 3));
    EXPECT_EQ(steps, 3u);
    ASSERT_TRUE(world.RunFixedUpdates(10, schedule, 0.125f, 3));
    EXPECT_EQ(steps, 6u);
    ASSERT_TRUE(world.RunFixedUpdates(0, schedule, 0.125f, 3));
    EXPECT_EQ(steps, 6u);
    ASSERT_TRUE(world.RunFixedUpdates(0.0625, schedule, 0.125f, 3));
    EXPECT_EQ(steps, 7u);

    EntityWorld other;
    ASSERT_TRUE(other.RunFixedUpdates(0.0625, schedule, 0.125f, 3));
    EXPECT_EQ(steps, 7u);
    EXPECT_EQ(other.GetEpoch(), 0u);
}


TEST(EntityFixedUpdate, InvalidTimingDoesNotAdvanceOrConsumeAccumulatedTime)
{
    EntityWorld world;
    const auto schedule = [](EntityWorld&) {};
    ASSERT_TRUE(world.RunFixedUpdates(0.0625, schedule, 0.125f));
    EXPECT_FALSE(world.RunFixedUpdates(-1, schedule, 0.125f));
    EXPECT_FALSE(world.RunFixedUpdates(std::numeric_limits<float>::infinity(), schedule, 0.125f));
    EXPECT_FALSE(world.RunFixedUpdates(std::numeric_limits<double>::quiet_NaN(), schedule, 0.125f));
    EXPECT_FALSE(world.RunFixedUpdates(1, schedule, 0));
    EXPECT_FALSE(world.RunFixedUpdates(1, schedule, std::numeric_limits<float>::infinity()));
    EXPECT_FALSE(world.RunFixedUpdates(1, schedule, 0.125f, 0));
    EXPECT_EQ(world.GetEpoch(), 0u);
    ASSERT_TRUE(world.RunFixedUpdates(0.0625, schedule, 0.125f));
    EXPECT_EQ(world.GetEpoch(), 1u);
}
