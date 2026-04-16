#pragma once

// Goal: A bitmapped free list allocator for managing valid slots. Array returns a handle with an index and a generation. 
// When a slot is freed, the generation is incremented
// Upon allocation, a fast lookup goes through top-level chunks via free-hint and within the chunk K-level bitmaps are used to find the first free slot
// (Optional) Make it async safe

#include "Defs.hpp"
#include "Types.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

// 2-level Bitmap Free List
template<typename T>
class THandleVector
{
    static_assert(std::is_move_constructible_v<T>, "THandleVector requires T to be move constructible.");
    static_assert(std::is_nothrow_move_constructible_v<T>, "THandleVector requires T to be nothrow move constructible for safe reallocation.");

private:

    std::byte* Elements = nullptr;
    u32* Generations = nullptr; 

    u64* L0Bitmaps = nullptr; // Lowest level bitmaps, each bit represents a slot
    u64* L1Bitmaps = nullptr; // Higher level bitmaps, each bit represents 64 L0Bitmaps - For every 4096 slots, one L1 bitmap is allocated

    size_t L0BitmapCount = 0;
    size_t L1BitmapCount = 0;

    size_t Capacity = 0;
    size_t Count = 0;
private:
    T* GetElementPtr(size_t Index)
    {
        return std::launder(reinterpret_cast<T*>(Elements + (Index * sizeof(T))));
    }

    const T* GetElementPtr(size_t Index) const
    {
        return std::launder(reinterpret_cast<const T*>(Elements + (Index * sizeof(T))));
    }

    template<typename Func>
    void ForEachUsedSlot(Func&& Visitor)
    {
        for(size_t l0Idx = 0; l0Idx < L0BitmapCount; ++l0Idx)
        {
            u64 mask = L0Bitmaps[l0Idx];
            while(mask != 0)
            {
                const size_t bit = std::countr_zero(mask);
                const size_t idx = (l0Idx * 64) + bit;
                Visitor(idx);
                mask &= (mask - 1); // clear least significant set bit
            }
        }
    }


public:

    THandleVector(size_t InitialCapacity = 1024)
    {
        Reserve(InitialCapacity);
    };

    ~THandleVector()
    {
        if(Elements)
        {
            ForEachUsedSlot([this](size_t idx)
            {
                std::destroy_at(GetElementPtr(idx));
            });

            ::operator delete[](Elements, std::align_val_t(alignof(T)));
        }

        delete[] Generations;
        delete[] L0Bitmaps;
        delete[] L1Bitmaps;
    };

    void Reserve(size_t NewCapacity)
    {
        if(NewCapacity > Capacity)
        {
            const size_t RequestedL0BitmapCount = (NewCapacity + 63) / 64;
            const size_t NewL1BitmapCount = (RequestedL0BitmapCount + 63) / 64;

            NewCapacity = (NewL1BitmapCount * 64) * 64; // Round up to nearest multiple of 4096

            const size_t NewL0BitmapCount = NewCapacity / 64;

            std::byte* NewElements = static_cast<std::byte*>(::operator new[](NewCapacity * sizeof(T), std::align_val_t(alignof(T))));
            u32* NewGenerations = new u32[NewCapacity];

            u64* NewL0Bitmaps = new u64[NewL0BitmapCount];
            u64* NewL1Bitmaps = new u64[NewL1BitmapCount];

            std::memset(NewGenerations, 0, NewCapacity * sizeof(u32));
            std::memset(NewL0Bitmaps, 0, NewL0BitmapCount * sizeof(u64));
            std::memset(NewL1Bitmaps, 0, NewL1BitmapCount * sizeof(u64));

            // Copy old data
            if(Elements)
            {
                ForEachUsedSlot([this, NewElements](size_t idx)
                {
                    T* OldElement = GetElementPtr(idx);
                    T* NewElement = std::launder(reinterpret_cast<T*>(NewElements + (idx * sizeof(T))));

                    std::construct_at(NewElement, std::move(*OldElement));
                    std::destroy_at(OldElement);
                });

                ::operator delete[](Elements, std::align_val_t(alignof(T)));
            }

            if(Generations)
            {
                std::copy(Generations, Generations + Capacity, NewGenerations);
                delete[] Generations;
            }

            if(L0Bitmaps)
            {
                std::copy(L0Bitmaps, L0Bitmaps + L0BitmapCount, NewL0Bitmaps);
                delete[] L0Bitmaps;
            }

            if(L1Bitmaps)
            {
                std::copy(L1Bitmaps, L1Bitmaps + L1BitmapCount, NewL1Bitmaps);
                delete[] L1Bitmaps;
            }

            Elements = NewElements;
            Generations = NewGenerations;

            L0Bitmaps = NewL0Bitmaps;
            L1Bitmaps = NewL1Bitmaps;
            L0BitmapCount = NewL0BitmapCount;
            L1BitmapCount = NewL1BitmapCount;
            Capacity = NewCapacity;
        }
    };

    T* Get(const THandle<T>& Handle)
    {
        if(Handle.Index < Capacity && Generations[Handle.Index] == Handle.Generation)
        {
            return GetElementPtr(Handle.Index);
        }else
        {
            return nullptr;
        }
    };

    void ReleaseHandle(THandle<T>& Handle)
    {
        if(Handle.Index < Capacity && Generations[Handle.Index] == Handle.Generation)
        {
            size_t l0Idx = Handle.Index / 64;
            size_t bitIdx = Handle.Index % 64;

            std::destroy_at(GetElementPtr(Handle.Index));

            // Mark the slot as free
            L0Bitmaps[l0Idx] &= ~(1ull << bitIdx);
            
            // Mark the L1 bitmap as having a free slot
            size_t l1Idx = l0Idx / 64;
            L1Bitmaps[l1Idx] &= ~(1ull << (l0Idx % 64));

            Handle.Generation = INVALID_U32; // Invalidate the handle
            Generations[Handle.Index]++; // Increment generation to invalidate old handles
            Handle.Index = INVALID_U32; // Invalidate the handle index
        }
    };

    void Clear()
    {
        if (!Elements)
        {
            return;
        }

        ForEachUsedSlot([this](size_t idx)
        {
            std::destroy_at(GetElementPtr(idx));
        });

        std::memset(L0Bitmaps, 0, L0BitmapCount * sizeof(u64));
        std::memset(L1Bitmaps, 0, L1BitmapCount * sizeof(u64));
        Count = 0;
    }

    template<typename... TArgs>
    void AllocateHandle(THandle<T>& OutHandle, TArgs&&... Args)
    {
        for(size_t l1Idx = 0; l1Idx < L1BitmapCount; ++l1Idx)
        {
            if(L1Bitmaps[l1Idx] != 0XFFFFFFFFFFFFFFFFull) // If not all bits are set, there is a free slot
            {
                size_t l0Idx = std::countr_one(L1Bitmaps[l1Idx]) + (l1Idx * 64);
                size_t freeBit = std::countr_one(L0Bitmaps[l0Idx]);
                size_t freeIdx = freeBit + (l0Idx * 64);

                // Mark the slot as used
                L0Bitmaps[l0Idx] |= (1ull << freeBit);
                
                if(L0Bitmaps[l0Idx] == 0xFFFFFFFFFFFFFFFFull) // If this L0 bitmap is now full, mark it in the L1 bitmap
                {
                    L1Bitmaps[l1Idx] |= (1ull << (l0Idx % 64));
                }

                std::construct_at(GetElementPtr(freeIdx), std::forward<TArgs>(Args)...);

                Generations[freeIdx]++; // Increment generation to invalidate old handles
                OutHandle.Index = static_cast<u32>(freeIdx);
                OutHandle.Generation = Generations[freeIdx];
                return;
            }
        }

        // No free slots, need to grow
        Reserve(Capacity * 2);
        return AllocateHandle(OutHandle, std::forward<TArgs>(Args)...);
    };

    void AllocateHandle(THandle<T>& OutHandle)
        requires std::default_initializable<T>
    {
        AllocateHandle<>(OutHandle);
    }

    bool ValidateInvariants() const
    {
        for(size_t l0Idx = 0; l0Idx < L0BitmapCount; ++l0Idx)
        {
            const size_t l1Idx = l0Idx / 64;
            const u64 l1Mask = (1ull << (l0Idx % 64));
            const bool l0Full = (L0Bitmaps[l0Idx] == 0xFFFFFFFFFFFFFFFFull);
            const bool l1MarkedFull = (L1Bitmaps[l1Idx] & l1Mask) != 0;

            if(l0Full != l1MarkedFull)
            {
                return false;
            }
        }

        return true;
    }

    size_t GetCapacity() const
    {
        return Capacity;
    }
};