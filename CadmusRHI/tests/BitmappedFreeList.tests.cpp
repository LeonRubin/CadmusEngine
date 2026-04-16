#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <random>
#include <unordered_set>
#include <vector>

#include "BitmappedFreeList.hpp"

TEST(THandleVector, AllocReleaseValidity)
{
    THandleVector<int> freeList(4096);

    std::vector<THandle<int>> handles;
    handles.reserve(1024);
    std::unordered_set<u32> usedIndices;

    for(size_t i = 0; i < 1024; ++i)
    {
        THandle<int> handle{};
        freeList.AllocateHandle(handle);

        EXPECT_TRUE(usedIndices.insert(handle.Index).second);

        int* element = freeList.Get(handle);
        ASSERT_NE(element, nullptr);
        *element = static_cast<int>(i);

        handles.push_back(handle);
    }

    EXPECT_TRUE(freeList.ValidateInvariants());

    for(size_t i = 0; i < handles.size(); i += 2)
    {
        THandle<int> staleHandle = handles[i];
        freeList.ReleaseHandle(handles[i]);

        EXPECT_EQ(freeList.Get(staleHandle), nullptr);
        usedIndices.erase(staleHandle.Index);
    }

    EXPECT_TRUE(freeList.ValidateInvariants());

    for(size_t i = 0; i < 512; ++i)
    {
        THandle<int> handle{};
        freeList.AllocateHandle(handle);

        EXPECT_TRUE(usedIndices.insert(handle.Index).second);
        EXPECT_NE(freeList.Get(handle), nullptr);
    }

    EXPECT_TRUE(freeList.ValidateInvariants());
}

TEST(THandleVector, RandomizedAllocFreeAlwaysValid)
{
    THandleVector<int> freeList(4096);

    std::mt19937 rng(42u);
    std::bernoulli_distribution allocateDist(0.6);

    std::vector<THandle<int>> activeHandles;
    activeHandles.reserve(4096);
    std::unordered_set<u32> activeIndices;

    constexpr size_t kOps = 20000;
    constexpr size_t kMaxLive = 3500;

    for(size_t op = 0; op < kOps; ++op)
    {
        const bool shouldAllocate = activeHandles.empty() || (allocateDist(rng) && activeHandles.size() < kMaxLive);

        if(shouldAllocate)
        {
            THandle<int> handle{};
            freeList.AllocateHandle(handle);

            EXPECT_TRUE(activeIndices.insert(handle.Index).second);
            EXPECT_NE(freeList.Get(handle), nullptr);
            activeHandles.push_back(handle);
        }
        else
        {
            std::uniform_int_distribution<size_t> releaseIdxDist(0, activeHandles.size() - 1);
            const size_t idx = releaseIdxDist(rng);

            THandle<int> staleHandle = activeHandles[idx];
            freeList.ReleaseHandle(activeHandles[idx]);

            EXPECT_EQ(freeList.Get(staleHandle), nullptr);
            activeIndices.erase(staleHandle.Index);

            activeHandles[idx] = activeHandles.back();
            activeHandles.pop_back();
        }

        if((op % 100) == 0)
        {
            EXPECT_TRUE(freeList.ValidateInvariants());
        }
    }

    for(THandle<int>& handle : activeHandles)
    {
        freeList.ReleaseHandle(handle);
    }

    EXPECT_TRUE(freeList.ValidateInvariants());
}

TEST(THandleVector, CapacityRoundsToBitmapHierarchy)
{
    THandleVector<int> freeList(1);
    EXPECT_EQ(freeList.GetCapacity(), 4096u);

    freeList.Reserve(5000);
    EXPECT_EQ(freeList.GetCapacity(), 8192u);

    EXPECT_TRUE(freeList.ValidateInvariants());
}

TEST(THandleVector, StaleHandleInvalidAfterReleaseAndReuse)
{
    THandleVector<int> freeList(64);

    THandle<int> first{};
    freeList.AllocateHandle(first);
    THandle<int> stale = first;

    ASSERT_NE(freeList.Get(first), nullptr);
    freeList.ReleaseHandle(first);

    EXPECT_EQ(first.Index, INVALID_U32);
    EXPECT_EQ(first.Generation, INVALID_U32);
    EXPECT_EQ(freeList.Get(stale), nullptr);

    THandle<int> second{};
    freeList.AllocateHandle(second);

    EXPECT_EQ(second.Index, stale.Index);
    EXPECT_GT(second.Generation, stale.Generation);
    EXPECT_NE(freeList.Get(second), nullptr);
    EXPECT_EQ(freeList.Get(stale), nullptr);
    EXPECT_TRUE(freeList.ValidateInvariants());
}

TEST(THandleVector, InvalidHandleReturnsNull)
{
    THandleVector<int> freeList(128);

    THandle<int> invalidIndex{INVALID_U32, 0};
    EXPECT_EQ(freeList.Get(invalidIndex), nullptr);

    THandle<int> handle{};
    freeList.AllocateHandle(handle);

    THandle<int> wrongGeneration = handle;
    wrongGeneration.Generation += 1;
    EXPECT_EQ(freeList.Get(wrongGeneration), nullptr);

    EXPECT_NE(freeList.Get(handle), nullptr);
    EXPECT_TRUE(freeList.ValidateInvariants());
}

TEST(THandleVector, DoubleReleaseOfSameHandleIsSafe)
{
    THandleVector<int> freeList(256);

    THandle<int> handle{};
    freeList.AllocateHandle(handle);
    freeList.ReleaseHandle(handle);
    freeList.ReleaseHandle(handle);

    EXPECT_EQ(handle.Index, INVALID_U32);
    EXPECT_EQ(handle.Generation, INVALID_U32);
    EXPECT_TRUE(freeList.ValidateInvariants());
}

TEST(THandleVector, GrowsWithoutDuplicateIndicesOnInitialAllocation)
{
    THandleVector<int> freeList(64);
    const size_t initialCapacity = freeList.GetCapacity();

    std::vector<THandle<int>> handles;
    handles.reserve(initialCapacity + 1);
    std::unordered_set<u32> indices;
    indices.reserve(initialCapacity + 1);

    for(size_t i = 0; i < initialCapacity + 1; ++i)
    {
        THandle<int> handle{};
        freeList.AllocateHandle(handle);
        EXPECT_TRUE(indices.insert(handle.Index).second);
        EXPECT_NE(freeList.Get(handle), nullptr);
        handles.push_back(handle);
    }

    EXPECT_GT(freeList.GetCapacity(), initialCapacity);
    EXPECT_TRUE(freeList.ValidateInvariants());
}

TEST(BitmappedFreeListPerf, AllocateReleaseThroughput)
{
    constexpr size_t kOps = 300000;
    THandleVector<int> freeList(kOps);

    std::vector<THandle<int>> handles;
    handles.reserve(kOps);

    const auto start = std::chrono::high_resolution_clock::now();

    for(size_t i = 0; i < kOps; ++i)
    {
        THandle<int> handle{};
        freeList.AllocateHandle(handle);
        handles.push_back(handle);
    }

    for(THandle<int>& handle : handles)
    {
        freeList.ReleaseHandle(handle);
    }

    const auto end = std::chrono::high_resolution_clock::now();

    const std::chrono::duration<double> elapsed = end - start;
    const double totalOperations = static_cast<double>(kOps * 2);
    const double opsPerSecond = totalOperations / elapsed.count();

    std::cout << "BitmappedFreeList throughput: " << opsPerSecond << " ops/sec ("
              << totalOperations << " ops in " << elapsed.count() << "s)" << std::endl;

    EXPECT_TRUE(freeList.ValidateInvariants());
    EXPECT_GT(opsPerSecond, 0.0);
}
