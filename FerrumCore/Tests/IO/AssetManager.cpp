#include <Core/IO/Artifact.h>
#include <Core/IO/AssetManager.h>
#include <Core/Jobs/WaitGroup.h>
#include <gtest/gtest.h>

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


    TEST_F(AssetManagerTest, DiscoversHardClosureWithoutTickAndIgnoresSoftLinks)
    {
        AssetRequest diamond = AssetManager::LoadAsset(kDiamond);
        diamond.Wait();
        ASSERT_EQ(diamond.GetResult(), AssetLoadResult::kSucceeded);
        EXPECT_FALSE(diamond.GetAssetSlot()->m_completed.load());
        EXPECT_EQ(AssetManager::FindAssetSlot(kSimple)->m_strongRefCount.load(), 1);
        EXPECT_EQ(AssetManager::FindAssetSlot(kChain)->m_strongRefCount.load(), 1);
        EXPECT_EQ(AssetManager::FindAssetSlot(kDiamondBranch)->m_strongRefCount.load(), 1);

        AssetRequest soft = AssetManager::LoadAsset(kSoft);
        soft.Wait();
        ASSERT_EQ(soft.GetResult(), AssetLoadResult::kSucceeded);
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

        first.Wait();
        second.Wait();
        ASSERT_EQ(first.GetResult(), AssetLoadResult::kSucceeded);
        ASSERT_EQ(second.GetResult(), AssetLoadResult::kSucceeded);
        EXPECT_EQ(AssetManager::FindAssetSlot(kSimple)->m_strongRefCount.load(), 2);
        EXPECT_EQ(AssetManager::GetMetadataReadCountForTests(kDiamond), 1);
        EXPECT_EQ(AssetManager::GetMetadataReadCountForTests(kSimple), 1);

        AssetRequest late = AssetManager::LoadAsset(kDiamond);
        late.Wait();
        EXPECT_EQ(late.GetResult(), AssetLoadResult::kSucceeded);
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
        diamondRequest.Wait();
        chainRequest.Wait();

        ASSERT_EQ(diamondRequest.GetResult(), AssetLoadResult::kSucceeded);
        ASSERT_EQ(chainRequest.GetResult(), AssetLoadResult::kSucceeded);
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
        request.Wait();
        ASSERT_EQ(request.GetResult(), AssetLoadResult::kSucceeded);

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
        lateRequest.Wait();
        ASSERT_EQ(lateRequest.GetResult(), AssetLoadResult::kSucceeded);
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
        request.Wait();
        ASSERT_EQ(request.GetResult(), AssetLoadResult::kSucceeded);
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
        survivor.Wait();

        EXPECT_EQ(survivor.GetResult(), AssetLoadResult::kSucceeded);
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
        request.Wait();
        ASSERT_EQ(request.GetResult(), AssetLoadResult::kFailed);
        EXPECT_EQ(request.GetAssetSlot()->m_strongRefCount.load(), 0);
    }
} // namespace FE::IO::Tests
