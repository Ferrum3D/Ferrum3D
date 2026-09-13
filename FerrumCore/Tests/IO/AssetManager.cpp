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
        const AssetID kStage4SelfCycle("16161616-1616-4616-8616-161616161616");
        const AssetID kMissingPathRoot("17171717-1717-4717-8717-171717171717");

        void PumpUntilCompleted(AssetRequest& request)
        {
            for (uint32_t iteration = 0; iteration < 10000 && !request.IsCompleted(); ++iteration)
            {
                AssetManager::Tick();
                Threading::Sleep(1);
            }
        }

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

            void CancelFinalize(AssetSlot&, void*) override {}

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
            void CancelFinalize(AssetSlot&, void*) override {}
            bool RequiresFinalizedDependencies() const override
            {
                return true;
            }
        };

        struct FailingStreamer final : Streamer
        {
            AssetFinalizeResult FinalizeAssetLoading(AssetSlot&, void*) override
            {
                return AssetFinalizeResult::kFailed;
            }
            AssetFinalizeResult PollFinalize(AssetSlot&, void*) override
            {
                return AssetFinalizeResult::kFailed;
            }
            void CancelFinalize(AssetSlot&, void*) override {}
        };

        struct NeverCompletingStreamer final : Streamer
        {
            AssetFinalizeResult FinalizeAssetLoading(AssetSlot&, void*) override
            {
                ++m_startCount;
                return AssetFinalizeResult::kPending;
            }

            AssetFinalizeResult PollFinalize(AssetSlot&, void*) override
            {
                return AssetFinalizeResult::kPending;
            }

            void CancelFinalize(AssetSlot&, void*) override
            {
                ++m_cancelCount;
            }

            uint32_t m_startCount = 0;
            uint32_t m_cancelCount = 0;
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
            Threading::Sleep(1);
        }

        ASSERT_TRUE(request.IsCompleted());
        ASSERT_EQ(request.GetResult(), AssetLoadResult::kSucceeded) << request.GetError().data();
        ASSERT_TRUE(root.IsReady());
        ASSERT_TRUE(leaf.IsReady());
        AssetRead<SyntheticAsset> rootRead = root.Read();
        AssetRead<SyntheticAsset> leafRead = leaf.Read();
        ASSERT_TRUE(rootRead);
        ASSERT_TRUE(leafRead);
        EXPECT_EQ(rootRead->m_value, 9);
        EXPECT_EQ(leafRead->m_value, 7);
        AssetHandle<SyntheticAsset> dependencyHandle = rootRead->m_dependency.GetAssetHandle();
        EXPECT_EQ(dependencyHandle.GetAssetSlot(), leaf.GetAssetSlot());
        AssetRead<SyntheticAsset> dependencyRead = dependencyHandle.Read();
        EXPECT_EQ(dependencyRead.Get(), leafRead.Get());
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
            Threading::Sleep(1);
        }
        ASSERT_EQ(streamer.m_startCount, 2);
        EXPECT_FALSE(cycleA.IsReady());
        EXPECT_FALSE(cycleB.IsReady());

        AssetManager::Tick();
        ASSERT_TRUE(cycleA.IsReady());
        ASSERT_TRUE(cycleB.IsReady());
        AssetRead<SyntheticAsset> cycleARead = cycleA.Read();
        AssetRead<SyntheticAsset> cycleBRead = cycleB.Read();
        ASSERT_TRUE(cycleARead);
        ASSERT_TRUE(cycleBRead);
        AssetRead<SyntheticAsset> dependencyARead = cycleARead->m_dependency.GetAssetHandle().Read();
        AssetRead<SyntheticAsset> dependencyBRead = cycleBRead->m_dependency.GetAssetHandle().Read();
        EXPECT_EQ(dependencyARead.Get(), cycleBRead.Get());
        EXPECT_EQ(dependencyBRead.Get(), cycleARead.Get());
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
            Threading::Sleep(1);
        }
        EXPECT_EQ(cycle.GetResult(), AssetLoadResult::kFailed);
        EXPECT_FALSE(cycle.GetError().empty());

        AssetRequest malformed = AssetManager::LoadAsset(kSimple);
        malformed.WaitForDiscovery();
        for (uint32_t iteration = 0; iteration < 10000 && !malformed.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }
        EXPECT_EQ(malformed.GetResult(), AssetLoadResult::kFailed);
        EXPECT_FALSE(malformed.GetError().empty());
        EXPECT_FALSE(malformed.GetAssetSlot()->m_completed.load());
    }


    TEST_F(AssetManagerTest, RejectsAnOrderedSelfReferenceCycle)
    {
        OrderedStreamer orderedStreamer;
        AssetManager::RegisterStreamer(Rtti::GetTypeID<SyntheticAsset>(), &orderedStreamer);
        AssetRequest request = AssetManager::LoadAsset(kStage4SelfCycle);
        request.WaitForDiscovery();
        PumpUntilCompleted(request);

        EXPECT_EQ(request.GetDiscoveryResult(), AssetLoadResult::kSucceeded);
        EXPECT_EQ(request.GetResult(), AssetLoadResult::kFailed);
        EXPECT_FALSE(request.GetError().empty());
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


    TEST_F(AssetManagerTest, TypedRootRejectsAnIncompatibleAssetType)
    {
        Link<SyntheticAsset> link;
        link.SetAssetID(kSimple);

        AssetRequest request = AssetManager::LoadAsset(link);
        request.WaitForDiscovery();

        EXPECT_EQ(request.GetDiscoveryResult(), AssetLoadResult::kFailed);
        EXPECT_FALSE(request.GetError().empty());
        EXPECT_EQ(request.GetAssetSlot()->m_strongRefCount.load(), 0);
    }


    TEST_F(AssetManagerTest, LinkResolvesItsSlotLazily)
    {
        Link<SyntheticAsset> link;
        link.SetAssetID(kStage4Leaf);
        EXPECT_EQ(link.GetAssetHandle().GetAssetSlot(), nullptr);

        AssetRequest request = AssetManager::LoadAsset(link);
        request.WaitForDiscovery();
        for (uint32_t iteration = 0; iteration < 10000 && !request.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }

        ASSERT_EQ(request.GetResult(), AssetLoadResult::kSucceeded) << request.GetError().data();
        AssetHandle<SyntheticAsset> handle = link.GetAssetHandle();
        EXPECT_EQ(handle.GetAssetSlot(), request.GetAssetSlot());
        AssetRead<SyntheticAsset> read = handle.Read();
        ASSERT_TRUE(read);
        EXPECT_EQ(read->m_value, 7);
    }


    TEST_F(AssetManagerTest, TickIsSafeWhileDiscoveryAppendsOperations)
    {
        Rc<WaitGroup> entered = WaitGroup::Create();
        Rc<WaitGroup> resume = WaitGroup::Create();
        AssetManager::SetDiscoveryBarrierForTests(entered.Get(), resume.Get());

        AssetRequest request = AssetManager::LoadAsset(kDiamond);
        entered->Wait();
        AssetManager::Tick();

        AssetManager::SetDiscoveryBarrierForTests(nullptr, nullptr);
        resume->Signal();
        for (uint32_t iteration = 0; iteration < 10000 && !request.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }

        ASSERT_TRUE(request.IsCompleted());
        EXPECT_EQ(request.GetDiscoveryResult(), AssetLoadResult::kSucceeded);
        EXPECT_EQ(request.GetResult(), AssetLoadResult::kFailed);
        EXPECT_FALSE(request.GetError().empty());
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


    TEST_F(AssetManagerTest, CyclesRemainNonOwningAcrossLateAcquisitions)
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
        AssetRequest lateRequest = AssetManager::LoadAsset(kCycleB);
        lateRequest.WaitForDiscovery();
        ASSERT_EQ(lateRequest.GetDiscoveryResult(), AssetLoadResult::kSucceeded);
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


    TEST_F(AssetManagerTest, MissingDependencyDiagnosticContainsTheCompletePath)
    {
        AssetRequest request = AssetManager::LoadAsset(kMissingPathRoot);
        request.WaitForDiscovery();
        ASSERT_EQ(request.GetDiscoveryResult(), AssetLoadResult::kFailed);

        const festd::string_view error = request.GetError();
        const auto rootPosition = error.find("17171717-1717-4717-8717-171717171717");
        ASSERT_NE(rootPosition, error.end());
        const auto intermediatePosition = error.find(rootPosition, "18181818-1818-4818-8818-181818181818");
        ASSERT_NE(intermediatePosition, error.end());
        EXPECT_NE(error.find(intermediatePosition, "99999999-9999-4999-8999-999999999999"), error.end());
    }


    TEST_F(AssetManagerTest, DefaultHandlesAreSafeToQuery)
    {
        AssetHandle<SyntheticAsset> handle;
        AssetLease<SyntheticAsset> lease;

        EXPECT_FALSE(handle.IsValid());
        EXPECT_FALSE(handle.IsReady());
        EXPECT_EQ(handle.GetGeneration(), 0);
        EXPECT_FALSE(handle.Read());
        EXPECT_FALSE(lease.IsValid());
        EXPECT_FALSE(lease.IsReady());
        EXPECT_EQ(lease.GetGeneration(), 0);
        EXPECT_FALSE(lease.Read());
    }


    TEST_F(AssetManagerTest, GenerationReadDelaysRetirementAndClosesNewAdmission)
    {
        AssetRequest request = AssetManager::LoadAsset(kStage4Leaf);
        for (uint32_t iteration = 0; iteration < 10000 && !request.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }
        ASSERT_EQ(request.GetResult(), AssetLoadResult::kSucceeded) << request.GetError().data();

        AssetHandle<SyntheticAsset> handle(request.GetAssetSlot());
        std::atomic<bool> readerEntered = false;
        std::atomic<bool> readerResume = false;
        std::atomic<uint32_t> observedValue = 0;
        std::thread reader([handle, &readerEntered, &readerResume, &observedValue] {
            AssetRead<SyntheticAsset> read = handle.Read();
            if (read)
                observedValue.store(read->m_value);
            readerEntered.store(true, std::memory_order_release);
            while (!readerResume.load(std::memory_order_acquire))
                Threading::Sleep(1);
            if (read)
                observedValue.store(read->m_value);
        });
        while (!readerEntered.load(std::memory_order_acquire))
            Threading::Sleep(1);
        EXPECT_EQ(observedValue.load(), 7);
        const uint32_t retiredBefore = AssetManager::GetRetiredGenerationCountForTests();

        request.Reset();
        AssetManager::Tick();
        EXPECT_TRUE(AssetManager::IsRetiringForTests(kStage4Leaf));
        EXPECT_FALSE(handle.IsReady());
        EXPECT_FALSE(handle.Read());
        EXPECT_EQ(observedValue.load(), 7);
        EXPECT_EQ(AssetManager::GetRetiredGenerationCountForTests(), retiredBefore);

        AssetManager::Tick();
        EXPECT_EQ(AssetManager::GetRetiredGenerationCountForTests(), retiredBefore);
        readerResume.store(true, std::memory_order_release);
        reader.join();
        AssetManager::Tick();
        EXPECT_EQ(AssetManager::GetRetiredGenerationCountForTests(), retiredBefore + 1);
    }


    TEST_F(AssetManagerTest, RenewedDemandAbortsRetirementBeforeDestruction)
    {
        AssetRequest first = AssetManager::LoadAsset(kStage4Leaf);
        for (uint32_t iteration = 0; iteration < 10000 && !first.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }
        ASSERT_EQ(first.GetResult(), AssetLoadResult::kSucceeded);

        AssetHandle<SyntheticAsset> handle(first.GetAssetSlot());
        AssetRead<SyntheticAsset> oldRead = handle.Read();
        const uint32_t generation = handle.GetGeneration();
        first.Reset();
        AssetManager::Tick();
        ASSERT_TRUE(AssetManager::IsRetiringForTests(kStage4Leaf));

        AssetRequest renewed = AssetManager::LoadAsset(kStage4Leaf);
        for (uint32_t iteration = 0; iteration < 10000 && !renewed.IsCompleted(); ++iteration)
            AssetManager::Tick();

        EXPECT_EQ(renewed.GetResult(), AssetLoadResult::kSucceeded);
        EXPECT_TRUE(handle.IsReady());
        EXPECT_EQ(handle.GetGeneration(), generation);
        EXPECT_EQ(handle.Read().Get(), oldRead.Get());
    }


    TEST_F(AssetManagerTest, FinalReleaseRetiresTheCompleteUnsharedHardClosure)
    {
        AssetRequest request = AssetManager::LoadAsset(kStage4Root);
        for (uint32_t iteration = 0; iteration < 10000 && !request.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }
        ASSERT_EQ(request.GetResult(), AssetLoadResult::kSucceeded);
        const uint32_t retiredBefore = AssetManager::GetRetiredGenerationCountForTests();

        request.Reset();
        AssetManager::Tick();
        EXPECT_EQ(AssetManager::GetRetiredGenerationCountForTests(), retiredBefore + 2);
        EXPECT_FALSE(AssetManager::FindAssetSlot(kStage4Root)->m_completed.load());
        EXPECT_FALSE(AssetManager::FindAssetSlot(kStage4Leaf)->m_completed.load());
    }


    TEST_F(AssetManagerTest, RetiredReferenceCycleCanBeLoadedAgain)
    {
        AssetRequest first = AssetManager::LoadAsset(kStage4CycleA);
        PumpUntilCompleted(first);
        ASSERT_EQ(first.GetResult(), AssetLoadResult::kSucceeded) << first.GetError().data();

        AssetHandle<SyntheticAsset> cycleA(first.GetAssetSlot());
        AssetHandle<SyntheticAsset> cycleB(AssetManager::FindAssetSlot(kStage4CycleB));
        const uint32_t firstGeneration = cycleA.GetGeneration();
        const uint32_t retiredBefore = AssetManager::GetRetiredGenerationCountForTests();
        first.Reset();
        AssetManager::Tick();
        EXPECT_EQ(AssetManager::GetRetiredGenerationCountForTests(), retiredBefore + 2);
        EXPECT_FALSE(cycleA.IsReady());
        EXPECT_FALSE(cycleB.IsReady());

        AssetRequest second = AssetManager::LoadAsset(kStage4CycleA);
        PumpUntilCompleted(second);
        ASSERT_EQ(second.GetResult(), AssetLoadResult::kSucceeded) << second.GetError().data();
        EXPECT_EQ(cycleA.GetGeneration(), firstGeneration + 1);
        EXPECT_TRUE(cycleA.Read());
        EXPECT_TRUE(cycleB.Read());
        EXPECT_EQ(AssetManager::GetMetadataReadCountForTests(kStage4CycleA), 2);
        EXPECT_EQ(AssetManager::GetMetadataReadCountForTests(kStage4CycleB), 2);
    }


    TEST_F(AssetManagerTest, ReloadRequiresAnExistingAcyclicPublishedGeneration)
    {
        Rc<WaitGroup> entered = WaitGroup::Create();
        Rc<WaitGroup> resume = WaitGroup::Create();
        AssetManager::SetDiscoveryBarrierForTests(entered.Get(), resume.Get());
        AssetRequest initial = AssetManager::LoadAsset(kStage4Leaf);
        entered->Wait();

        EXPECT_FALSE(AssetManager::ReloadAsset(kStage4Leaf).IsValid());
        AssetManager::SetDiscoveryBarrierForTests(nullptr, nullptr);
        resume->Signal();
        PumpUntilCompleted(initial);
        ASSERT_EQ(initial.GetResult(), AssetLoadResult::kSucceeded);

        AssetRequest cycle = AssetManager::LoadAsset(kStage4CycleA);
        PumpUntilCompleted(cycle);
        ASSERT_EQ(cycle.GetResult(), AssetLoadResult::kSucceeded);
        EXPECT_FALSE(AssetManager::ReloadAsset(kStage4CycleA).IsValid());
    }


    TEST_F(AssetManagerTest, ReplacementPublishesBesideAReaderThenRetiresTheOldGeneration)
    {
        AssetRequest initial = AssetManager::LoadAsset(kStage4Leaf);
        for (uint32_t iteration = 0; iteration < 10000 && !initial.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }
        ASSERT_EQ(initial.GetResult(), AssetLoadResult::kSucceeded);

        AssetHandle<SyntheticAsset> handle(initial.GetAssetSlot());
        AssetRead<SyntheticAsset> oldRead = handle.Read();
        ASSERT_TRUE(oldRead);
        const SyntheticAsset* oldInstance = oldRead.Get();
        const uint32_t oldGeneration = handle.GetGeneration();
        const uint32_t retiredBefore = AssetManager::GetRetiredGenerationCountForTests();

        AssetRequest replacement = AssetManager::ReloadAsset(kStage4Leaf);
        for (uint32_t iteration = 0; iteration < 10000 && !replacement.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }

        ASSERT_EQ(replacement.GetResult(), AssetLoadResult::kSucceeded) << replacement.GetError().data();
        AssetRead<SyntheticAsset> newRead = handle.Read();
        ASSERT_TRUE(newRead);
        EXPECT_NE(newRead.Get(), oldInstance);
        EXPECT_EQ(newRead->m_value, oldRead->m_value);
        EXPECT_EQ(handle.GetGeneration(), oldGeneration + 1);
        EXPECT_EQ(AssetManager::GetRetiredGenerationCountForTests(), retiredBefore);

        oldRead.Reset();
        AssetManager::Tick();
        EXPECT_EQ(AssetManager::GetRetiredGenerationCountForTests(), retiredBefore + 1);
    }


    TEST_F(AssetManagerTest, FailedReplacementPreservesTheUsableGeneration)
    {
        AssetRequest initial = AssetManager::LoadAsset(kStage4Leaf);
        for (uint32_t iteration = 0; iteration < 10000 && !initial.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }
        ASSERT_EQ(initial.GetResult(), AssetLoadResult::kSucceeded);

        AssetHandle<SyntheticAsset> handle(initial.GetAssetSlot());
        AssetRead<SyntheticAsset> initialRead = handle.Read();
        const SyntheticAsset* instance = initialRead.Get();
        const uint32_t generation = handle.GetGeneration();
        FailingStreamer failingStreamer;
        AssetManager::RegisterStreamer(Rtti::GetTypeID<SyntheticAsset>(), &failingStreamer);

        AssetRequest replacement = AssetManager::ReloadAsset(kStage4Leaf);
        for (uint32_t iteration = 0; iteration < 10000 && !replacement.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }

        EXPECT_EQ(replacement.GetResult(), AssetLoadResult::kFailed);
        EXPECT_TRUE(handle.IsReady());
        EXPECT_EQ(handle.Read().Get(), instance);
        EXPECT_EQ(handle.GetGeneration(), generation);
    }


    TEST_F(AssetManagerTest, SupersededReplacementCannotPublishAStaleCompletion)
    {
        AssetRequest initial = AssetManager::LoadAsset(kStage4Leaf);
        for (uint32_t iteration = 0; iteration < 10000 && !initial.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }
        ASSERT_EQ(initial.GetResult(), AssetLoadResult::kSucceeded);

        Rc<WaitGroup> oldEntered = WaitGroup::Create();
        Rc<WaitGroup> oldResume = WaitGroup::Create();
        AssetManager::SetDiscoveryBarrierForTests(oldEntered.Get(), oldResume.Get());
        AssetRequest oldReplacement = AssetManager::ReloadAsset(kStage4Leaf);
        oldEntered->Wait();

        Rc<WaitGroup> newEntered = WaitGroup::Create();
        Rc<WaitGroup> newResume = WaitGroup::Create();
        AssetManager::SetDiscoveryBarrierForTests(newEntered.Get(), newResume.Get());
        AssetRequest newReplacement = AssetManager::ReloadAsset(kStage4Leaf);
        newEntered->Wait();
        AssetManager::SetDiscoveryBarrierForTests(nullptr, nullptr);
        newResume->Signal();
        for (uint32_t iteration = 0; iteration < 10000 && !newReplacement.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }
        ASSERT_EQ(newReplacement.GetResult(), AssetLoadResult::kSucceeded);
        const uint32_t newGeneration = newReplacement.GetAssetSlot()->m_generation.load();

        oldResume->Signal();
        for (uint32_t iteration = 0; iteration < 10000 && !oldReplacement.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }
        EXPECT_EQ(oldReplacement.GetResult(), AssetLoadResult::kFailed);
        EXPECT_EQ(newReplacement.GetAssetSlot()->m_generation.load(), newGeneration);
    }


    TEST_F(AssetManagerTest, CanceledReplacementReclaimsItsCandidateWithoutDisturbingCurrentGeneration)
    {
        AssetRequest initial = AssetManager::LoadAsset(kStage4Leaf);
        for (uint32_t iteration = 0; iteration < 10000 && !initial.IsCompleted(); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }
        ASSERT_EQ(initial.GetResult(), AssetLoadResult::kSucceeded);
        AssetHandle<SyntheticAsset> handle(initial.GetAssetSlot());
        AssetRead<SyntheticAsset> initialRead = handle.Read();
        const SyntheticAsset* instance = initialRead.Get();
        const uint32_t generation = handle.GetGeneration();

        Rc<WaitGroup> entered = WaitGroup::Create();
        Rc<WaitGroup> resume = WaitGroup::Create();
        AssetManager::SetDiscoveryBarrierForTests(entered.Get(), resume.Get());
        AssetRequest replacement = AssetManager::ReloadAsset(kStage4Leaf);
        entered->Wait();
        replacement.Cancel();
        AssetManager::SetDiscoveryBarrierForTests(nullptr, nullptr);
        resume->Signal();

        EXPECT_EQ(replacement.GetResult(), AssetLoadResult::kCanceled);
        for (uint32_t iteration = 0; iteration < 100; ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }
        EXPECT_EQ(handle.Read().Get(), instance);
        EXPECT_EQ(handle.GetGeneration(), generation);
    }


    TEST_F(AssetManagerTest, SharedDependencyRemainsPublishedUntilItsFinalDemandEnds)
    {
        AssetRequest root = AssetManager::LoadAsset(kStage4Root);
        AssetRequest leaf = AssetManager::LoadAsset(kStage4Leaf);
        for (uint32_t iteration = 0; iteration < 10000 && (!root.IsCompleted() || !leaf.IsCompleted()); ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }
        ASSERT_EQ(root.GetResult(), AssetLoadResult::kSucceeded);
        ASSERT_EQ(leaf.GetResult(), AssetLoadResult::kSucceeded);
        AssetHandle<SyntheticAsset> leafHandle(leaf.GetAssetSlot());

        root.Reset();
        AssetManager::Tick();
        EXPECT_TRUE(leafHandle.IsReady());
        EXPECT_EQ(leaf.GetAssetSlot()->m_strongRefCount.load(), 1);

        leaf.Reset();
        AssetManager::Tick();
        EXPECT_FALSE(leafHandle.IsReady());
    }


    TEST(AssetManagerLifetime, ShutdownDrainsPendingWorkAndInvalidatesOutstandingHandlesAndRequests)
    {
        ArtifactStore::SetCatalogSource(Path(FE_CORE_TEST_SOURCE_DIR) / "Fixtures/Artifacts");
        AssetManager::Init();
        AssetRequest request = AssetManager::LoadAsset(kStage4Root);
        AssetHandle<SyntheticAsset> handle(request.GetAssetSlot());

        AssetManager::Shutdown();

        EXPECT_FALSE(request.IsValid());
        EXPECT_TRUE(request.IsCompleted());
        EXPECT_FALSE(handle.IsReady());
        request.Reset();
    }


    TEST(AssetManagerLifetime, ShutdownCancelsPendingFinalizationBeforeDestroyingTheCandidate)
    {
        ArtifactStore::SetCatalogSource(Path(FE_CORE_TEST_SOURCE_DIR) / "Fixtures/Artifacts");
        AssetManager::Init();
        NeverCompletingStreamer streamer;
        AssetManager::RegisterStreamer(Rtti::GetTypeID<SyntheticAsset>(), &streamer);
        AssetRequest request = AssetManager::LoadAsset(kStage4Leaf);
        for (uint32_t iteration = 0; iteration < 10000 && streamer.m_startCount == 0; ++iteration)
        {
            AssetManager::Tick();
            Threading::Sleep(1);
        }
        ASSERT_EQ(streamer.m_startCount, 1);

        AssetManager::Shutdown();

        EXPECT_EQ(streamer.m_cancelCount, 1);
        EXPECT_FALSE(request.IsValid());
        EXPECT_EQ(request.GetResult(), AssetLoadResult::kCanceled);
        request.Reset();
    }
} // namespace FE::IO::Tests
