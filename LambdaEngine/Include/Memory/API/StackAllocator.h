#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	/*
	* MemoryArena
	*/

	class MemoryArena
	{
	public:
		inline MemoryArena(uint32 sizeInBytes = 4096)
			: m_pMemory(nullptr)
			, m_Offset(0)
			, m_SizeInBytes(sizeInBytes)
		{
			m_pMemory = Malloc::AllocateType<byte>(m_SizeInBytes);
			ZERO_MEMORY(m_pMemory, m_SizeInBytes);

			Reset();
		}

		inline ~MemoryArena()
		{
			Malloc::Free(m_pMemory);
		}

		FORCEINLINE void* Allocate(uint32 size)
		{
			if (!CanAllocate(size))
			{
				return nullptr;
			}

			byte* pResult = m_pMemory + m_Offset;
			m_Offset += size;
			return pResult;
		}

		FORCEINLINE void Pop(uint32 size)
		{
			if (CanPop(size))
			{
				m_Offset -= size;
			}
		}

		FORCEINLINE void Reset()
		{
			m_Offset = 0;
		}

		FORCEINLINE bool CanAllocate(uint32 size) const
		{
			return (m_Offset + size) < m_SizeInBytes;
		}

		FORCEINLINE bool CanPop(uint32 size) const
		{
			return m_Offset >= size;
		}

	private:
		byte*	m_pMemory;
		uint32	m_Offset;
		uint32	m_SizeInBytes;
	};

	/*
	* StackAllocator -> Can resize, but cannot allocate arbitrarily
	*/

	class StackAllocator
	{
	public:
		StackAllocator(uint32 sizePerArena = 4096);
		~StackAllocator() = default;

		// Allocates a certain amounts of bytes
		void* Push(uint32 size);

		// Removes the last element
		void Pop();

		// Resets the whole stack
		void Reset();

		template<typename T>
		FORCEINLINE void* Allocate()
		{
			return Allocate(sizeof(T));
		}

	private:
		uint32 m_LastSize;
		uint32 m_ArenaIndex;
		uint32 m_SizePerArena;

		MemoryArena* m_pCurrentArena;
		TArray<MemoryArena> m_Arenas;
	};
}
