#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/Jobs/WaitGroup.h>
#include <Core/Threading/Thread.h>
#include <IO/AssetTestTypes.h>
#include <gtest/gtest.h>
#include <thread>

namespace FE::IO::Tests
{
    namespace
    {
        const AssetID kSimple("11111111-1111-4111-8111-111111111111");
        const AssetID kChain("22222222-2222-4222-8222-222222222222");
        const AssetID kDiamond("33333333-3333-4333-8333-333333333333");
        const AssetID kDiamondBranch("44444444-4444-4444-8444-444444444444");
        const AssetID kCycleA("66666666-6666-4666-8666-666666666666");
        const AssetID kCycleB("77777777-7777-4777-8777-777777777777");
        const AssetID kSoft("88888888-8888-4888-8888-888888888888");
        const AssetID kStage4Leaf("12121212-1212-4212-8212-121212121212");
        const AssetID kStage4Root("13131313-1313-4313-8313-131313131313");
        const AssetID kStage4CycleA("14141414-1414-4414-8414-141414141414");
        const AssetID kStage4CycleB("15151515-1515-4515-8515-151515151515");

        struct PendingStreamer final : Streamer
        {
            AssetFinalizeResult FinalizeAssetLoading(AssetSlot&, void*) override
            {
                m_startedOnMainThread &= Threading::IsMainThread();
                ++m_startCount;
                return AssetFinalizeResult::kPending;
            }

            AssetFinalizeResult PollFinalize(AssetSlot&, void*) override
            {
                m_polledOnMainThread &= Threading::IsMainThread();
                return AssetFinalizeResult::kSucceeded;
            }

            bool m_startedOnMainThread = true;
            bool m_polledOnMainThread = true;
            uint32_t m_startCount = 0;
        };

        struct OrderedStreamer final : Streamer
        {
            AssetFinalizeResult FinalizeAssetLoading(AssetSlot&, void*) override
            {
                return AssetFinalizeResult::kSucceeded;
            }
            AssetFinalizeResult PollFinalize(AssetSlot&, void*) override
            {
                return AssetFinalizeResult::kSucceeded;
            }
            bool RequiresFinalizedDependencies() const override
            {
                return true;
            }
        };

        struct AssetManagerTest : testing::Test
        {
            void SetUp() override
            {
                ArtifactStore::SetCatalogSource(Path(FE_CORE_TEST_SOURCE_DIR) / "Fixtures/Artifacts");
                AssetManager::Init();
            }

            void TearDown() override
            {
                AssetManager::SetDiscoveryBarrierForTests(nullptr, nullptr);
                AssetManager::Shutdown();
            }
        };
    } // namespace


    TEST_F(AssetManagerTest, DeserializesBindsFinalizesAndPublishesHardClosure)
    {
        PendingStreamer streamer;
        AssetManager::RegisterStreamer(Rtti::GetTypeID<SyntheticAsset>(), &streamer);
        AssetRequest request = AssetManager::LoadAsset(kStage4Root);
        request.WaitForDiscovery();
        ASSERT_EQ(request.GetDiscoveryResult(), AssetLoadResult::kSucceeded);

        AssetHandle<SyntheticAsset> root(request.GetAssetSlot());
        AssetHandle<SyntheticAsset> leaf(AssetManager::FindAssetSlot(kStage4Leaf));
        EXPECT_FALSE(root.IsReady());
        EXPECT_FALSE(leaf.IsReady());

        for (uint32_t iteration = 0; iteration < 10000 && !request.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            std::this_thread::yield();
        }

        ASSERT_TRUE(request.IsCompleted());
        ASSERT_EQ(request.GetResult(), AssetLoadResult::kSucceeded) << request.GetError().data();
        ASSERT_TRUE(root.IsReady());
        ASSERT_TRUE(leaf.IsReady());
        ASSERT_NE(root.Get(), nullptr);
        ASSERT_NE(leaf.Get(), nullptr);
        EXPECT_EQ(root.Get()->m_value, 9);
        EXPECT_EQ(leaf.Get()->m_value, 7);
        EXPECT_EQ(root.Get()->m_dependency.GetAssetHandle().GetAssetSlot(), leaf.GetAssetSlot());
        EXPECT_EQ(root.Get()->m_dependency.GetAssetHandle().Get(), leaf.Get());
        EXPECT_TRUE(streamer.m_startedOnMainThread);
        EXPECT_TRUE(streamer.m_polledOnMainThread);
    }


    TEST_F(AssetManagerTest, PublishesReferenceCycleThroughOneVisibilityGate)
    {
        PendingStreamer streamer;
        AssetManager::RegisterStreamer(Rtti::GetTypeID<SyntheticAsset>(), &streamer);
        AssetRequest request = AssetManager::LoadAsset(kStage4CycleA);
        request.WaitForDiscovery();
        AssetHandle<SyntheticAsset> cycleA(request.GetAssetSlot());
        AssetHandle<SyntheticAsset> cycleB(AssetManager::FindAssetSlot(kStage4CycleB));

        for (uint32_t iteration = 0; iteration < 10000 && streamer.m_startCount != 2; ++iteration)
        {
            AssetManager::Tick();
            std::this_thread::yield();
        }
        ASSERT_EQ(streamer.m_startCount, 2);
        EXPECT_FALSE(cycleA.IsReady());
        EXPECT_FALSE(cycleB.IsReady());

        AssetManager::Tick();
        ASSERT_TRUE(cycleA.IsReady());
        ASSERT_TRUE(cycleB.IsReady());
        EXPECT_EQ(cycleA.Get()->m_dependency.GetAssetHandle().Get(), cycleB.Get());
        EXPECT_EQ(cycleB.Get()->m_dependency.GetAssetHandle().Get(), cycleA.Get());
    }


    TEST_F(AssetManagerTest, RejectsFinalizationOrderCycleAndBadPayload)
    {
        OrderedStreamer orderedStreamer;
        AssetManager::RegisterStreamer(Rtti::GetTypeID<SyntheticAsset>(), &orderedStreamer);
        AssetRequest cycle = AssetManager::LoadAsset(kStage4CycleA);
        cycle.WaitForDiscovery();
        for (uint32_t iteration = 0; iteration < 10000 && !cycle.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            std::this_thread::yield();
        }
        EXPECT_EQ(cycle.GetResult(), AssetLoadResult::kFailed);
        EXPECT_FALSE(cycle.GetError().empty());

        AssetRequest malformed = AssetManager::LoadAsset(kSimple);
        malformed.WaitForDiscovery();
        for (uint32_t iteration = 0; iteration < 10000 && !malformed.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            std::this_thread::yield();
        }
        EXPECT_EQ(malformed.GetResult(), AssetLoadResult::kFailed);
        EXPECT_FALSE(malformed.GetError().empty());
        EXPECT_FALSE(malformed.GetAssetSlot()->m_completed.load());
    }


    TEST_F(AssetManagerTest, DiscoversHardClosureWithoutTickAndIgnoresSoftLinks)
    {
        AssetRequest diamond = AssetManager::LoadAsset(kDiamond);
        diamond.WaitForDiscovery();
        ASSERT_EQ(diamond.GetDiscoveryResult(), AssetLoadResult::kSucceeded);
        EXPECT_FALSE(diamond.GetAssetSlot()->m_completed.load());
        EXPECT_EQ(AssetManager::FindAssetSlot(kSimple)->m_strongRefCount.load(), 1);
        EXPECT_EQ(AssetManager::FindAssetSlot(kChain)->m_strongRefCount.load(), 1);
        EXPECT_EQ(AssetManager::FindAssetSlot(kDiamondBranch)->m_strongRefCount.load(), 1);

        AssetRequest soft = AssetManager::LoadAsset(kSoft);
        soft.WaitForDiscovery();
        ASSERT_EQ(soft.GetDiscoveryResult(), AssetLoadResult::kSucceeded);
        EXPECT_EQ(AssetManager::FindAssetSlot(AssetID("99999999-9999-4999-8999-999999999999")), nullptr);
    }


    TEST_F(AssetManagerTest, CoalescesConcurrentAndLateJoiningOperations)
    {
        Rc<WaitGroup> entered = WaitGroup::Create();
        Rc<WaitGroup> resume = WaitGroup::Create();
        AssetManager::SetDiscoveryBarrierForTests(entered.Get(), resume.Get());

        AssetRequest first = AssetManager::LoadAsset(kDiamond);
        entered->Wait();

        Rc<WaitGroup> dependenciesEntered = WaitGroup::Create(2);
        Rc<WaitGroup> dependenciesResume = WaitGroup::Create();
        AssetManager::SetDiscoveryBarrierForTests(dependenciesEntered.Get(), dependenciesResume.Get());
        resume->Signal();
        dependenciesEntered->Wait();

        // The root metadata is already pinned and its two direct dependency operations are in flight.
        AssetRequest second = AssetManager::LoadAsset(kDiamond);
        EXPECT_EQ(first.GetAssetSlot(), second.GetAssetSlot());
        EXPECT_EQ(first.GetAssetSlot()->m_strongRefCount.load(), 2);
        AssetManager::SetDiscoveryBarrierForTests(nullptr, nullptr);
        dependenciesResume->Signal();

        first.WaitForDiscovery();
        second.WaitForDiscovery();
        ASSERT_EQ(first.GetDiscoveryResult(), AssetLoadResult::kSucceeded);
        ASSERT_EQ(second.GetDiscoveryResult(), AssetLoadResult::kSucceeded);
        EXPECT_EQ(AssetManager::FindAssetSlot(kSimple)->m_strongRefCount.load(), 2);
        EXPECT_EQ(AssetManager::GetMetadataReadCountForTests(kDiamond), 1);
        EXPECT_EQ(AssetManager::GetMetadataReadCountForTests(kSimple), 1);

        AssetRequest late = AssetManager::LoadAsset(kDiamond);
        late.WaitForDiscovery();
        EXPECT_EQ(late.GetDiscoveryResult(), AssetLoadResult::kSucceeded);
        EXPECT_EQ(AssetManager::GetMetadataReadCountForTests(kDiamond), 1);
        EXPECT_EQ(AssetManager::GetMetadataReadCountForTests(kSimple), 1);
        EXPECT_EQ(AssetManager::FindAssetSlot(kSimple)->m_strongRefCount.load(), 3);
    }


    TEST_F(AssetManagerTest, DirectRootJoinsSubgraphAlreadyLoadingAsDependency)
    {
        Rc<WaitGroup> rootEntered = WaitGroup::Create();
        Rc<WaitGroup> rootResume = WaitGroup::Create();
        AssetManager::SetDiscoveryBarrierForTests(rootEntered.Get(), rootResume.Get());

        AssetRequest diamondRequest = AssetManager::LoadAsset(kDiamond);
        rootEntered->Wait();

        Rc<WaitGroup> dependenciesEntered = WaitGroup::Create(2);
        Rc<WaitGroup> dependenciesResume = WaitGroup::Create();
        AssetManager::SetDiscoveryBarrierForTests(dependenciesEntered.Get(), dependenciesResume.Get());
        rootResume->Signal();
        dependenciesEntered->Wait();

        // kChain is already retained and loading as part of kDiamond's closure when it becomes a direct root.
        AssetRequest chainRequest = AssetManager::LoadAsset(kChain);
        EXPECT_EQ(chainRequest.GetAssetSlot(), AssetManager::FindAssetSlot(kChain));
        EXPECT_EQ(chainRequest.GetAssetSlot()->m_strongRefCount.load(), 2);
        EXPECT_EQ(AssetManager::GetMetadataReadCountForTests(kChain), 1);

        AssetManager::SetDiscoveryBarrierForTests(nullptr, nullptr);
        dependenciesResume->Signal();
        diamondRequest.WaitForDiscovery();
        chainRequest.WaitForDiscovery();

        ASSERT_EQ(diamondRequest.GetDiscoveryResult(), AssetLoadResult::kSucceeded);
        ASSERT_EQ(chainRequest.GetDiscoveryResult(), AssetLoadResult::kSucceeded);
        EXPECT_EQ(AssetManager::GetMetadataReadCountForTests(kChain), 1);
        EXPECT_EQ(AssetManager::GetMetadataReadCountForTests(kSimple), 1);
        EXPECT_EQ(AssetManager::FindAssetSlot(kSimple)->m_strongRefCount.load(), 2);

        diamondRequest.Reset();
        EXPECT_EQ(AssetManager::FindAssetSlot(kDiamond)->m_strongRefCount.load(), 0);
        EXPECT_EQ(AssetManager::FindAssetSlot(kDiamondBranch)->m_strongRefCount.load(), 0);
        EXPECT_EQ(AssetManager::FindAssetSlot(kChain)->m_strongRefCount.load(), 1);
        EXPECT_EQ(AssetManager::FindAssetSlot(kSimple)->m_strongRefCount.load(), 1);

        chainRequest.Reset();
        EXPECT_EQ(AssetManager::FindAssetSlot(kChain)->m_strongRefCount.load(), 0);
        EXPECT_EQ(AssetManager::FindAssetSlot(kSimple)->m_strongRefCount.load(), 0);
    }


    TEST_F(AssetManagerTest, CyclesBecomeNonOwningReferenceGroups)
    {
        AssetRequest request = AssetManager::LoadAsset(kCycleA);
        request.WaitForDiscovery();
        ASSERT_EQ(request.GetDiscoveryResult(), AssetLoadResult::kSucceeded);

        AssetSlot* cycleA = AssetManager::FindAssetSlot(kCycleA);
        AssetSlot* cycleB = AssetManager::FindAssetSlot(kCycleB);
        ASSERT_NE(cycleA, nullptr);
        ASSERT_NE(cycleB, nullptr);
        EXPECT_EQ(cycleA->m_strongRefCount.load(), 1);
        EXPECT_EQ(cycleB->m_strongRefCount.load(), 1);
        EXPECT_NE(cycleA->m_referenceGroup.load(), kInvalidIndex);
        EXPECT_EQ(cycleA->m_referenceGroup.load(), cycleB->m_referenceGroup.load());

        const uint32_t referenceGroup = cycleA->m_referenceGroup.load();
        AssetRequest lateRequest = AssetManager::LoadAsset(kCycleB);
        lateRequest.WaitForDiscovery();
        ASSERT_EQ(lateRequest.GetDiscoveryResult(), AssetLoadResult::kSucceeded);
        EXPECT_EQ(cycleA->m_referenceGroup.load(), referenceGroup);
        EXPECT_EQ(cycleB->m_referenceGroup.load(), referenceGroup);

        request.Reset();
        lateRequest.Reset();
        EXPECT_EQ(cycleA->m_strongRefCount.load(), 0);
        EXPECT_EQ(cycleB->m_strongRefCount.load(), 0);
    }


    TEST_F(AssetManagerTest, RequestCopiesShareOneResidencyRecord)
    {
        AssetRequest request = AssetManager::LoadAsset(kChain);
        AssetRequest copy = request;
        request.WaitForDiscovery();
        ASSERT_EQ(request.GetDiscoveryResult(), AssetLoadResult::kSucceeded);
        EXPECT_EQ(AssetManager::FindAssetSlot(kSimple)->m_strongRefCount.load(), 1);

        request.Reset();
        EXPECT_EQ(AssetManager::FindAssetSlot(kSimple)->m_strongRefCount.load(), 1);
        copy.Reset();
        EXPECT_EQ(AssetManager::FindAssetSlot(kSimple)->m_strongRefCount.load(), 0);
    }


    TEST_F(AssetManagerTest, CancelingOneConsumerPreservesSharedWork)
    {
        Rc<WaitGroup> entered = WaitGroup::Create();
        Rc<WaitGroup> resume = WaitGroup::Create();
        AssetManager::SetDiscoveryBarrierForTests(entered.Get(), resume.Get());
        AssetRequest canceled = AssetManager::LoadAsset(kChain);
        entered->Wait();
        AssetRequest survivor = AssetManager::LoadAsset(kChain);

        canceled.Cancel();
        EXPECT_TRUE(canceled.IsCanceled());
        EXPECT_EQ(AssetManager::FindAssetSlot(kChain)->m_strongRefCount.load(), 1);
        AssetManager::SetDiscoveryBarrierForTests(nullptr, nullptr);
        resume->Signal();
        survivor.WaitForDiscovery();

        EXPECT_EQ(survivor.GetDiscoveryResult(), AssetLoadResult::kSucceeded);
        EXPECT_EQ(AssetManager::FindAssetSlot(kSimple)->m_strongRefCount.load(), 1);
        EXPECT_EQ(AssetManager::GetMetadataReadCountForTests(kChain), 1);
    }


    TEST_F(AssetManagerTest, ReleaseDuringDiscoveryUnwindsExactlyOnce)
    {
        Rc<WaitGroup> entered = WaitGroup::Create();
        Rc<WaitGroup> resume = WaitGroup::Create();
        AssetManager::SetDiscoveryBarrierForTests(entered.Get(), resume.Get());
        AssetRequest request = AssetManager::LoadAsset(kChain);
        entered->Wait();
        AssetSlot* root = request.GetAssetSlot();
        request.Reset();
        EXPECT_EQ(root->m_strongRefCount.load(), 0);

        AssetManager::SetDiscoveryBarrierForTests(nullptr, nullptr);
        resume->Signal();
    }


    TEST_F(AssetManagerTest, MissingHardMetadataFailsAndRollsBackMembership)
    {
        const AssetID missing("99999999-9999-4999-8999-999999999999");
        AssetRequest request = AssetManager::LoadAsset(missing);
        request.WaitForDiscovery();
        ASSERT_EQ(request.GetDiscoveryResult(), AssetLoadResult::kFailed);
        EXPECT_EQ(request.GetAssetSlot()->m_strongRefCount.load(), 0);
    }
} // namespace FE::IO::Tests
