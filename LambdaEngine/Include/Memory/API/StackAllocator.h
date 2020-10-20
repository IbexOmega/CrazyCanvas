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
		DECL_REMOVE_COPY(MemoryArena);

		inline MemoryArena(uint32 sizeInBytes = 4096)
			: m_pMemory(nullptr)
			, m_Offset(0)
			, m_SizeInBytes(sizeInBytes)
		{
			m_pMemory = Malloc::AllocateType<byte>(m_SizeInBytes);

#if LAMBDA_DEVELOPMENT
			ZERO_MEMORY(m_pMemory, m_SizeInBytes);
#endif
			Reset();
		}

		inline MemoryArena(MemoryArena&& other)
			: m_pMemory(other.m_pMemory)
			, m_Offset(other.m_Offset)
			, m_SizeInBytes(other.m_SizeInBytes)
		{
			other.m_pMemory		= nullptr;
			other.m_Offset		= 0;
			other.m_SizeInBytes	= 0;
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
		DECL_REMOVE_COPY(StackAllocator);
		DECL_REMOVE_MOVE(StackAllocator);

		StackAllocator(uint32 sizePerArena = 4096);
		~StackAllocator() = default;

		// Allocates a certain amounts of bytes
		void* Push(uint32 size);

		// Removes the last element
		void Pop(uint32 size);

		// Resets the whole stack
		void Reset();

		FORCEINLINE bool CanPop(uint32 size) const
		{
			VALIDATE(m_pCurrentArena != nullptr);
			return m_pCurrentArena->CanPop(size);
		}

		template<typename T>
		FORCEINLINE void* Allocate()
		{
			return Push(sizeof(T));
		}

	private:
		uint32 m_ArenaIndex;
		uint32 m_SizePerArena;

		MemoryArena* m_pCurrentArena;
		TArray<MemoryArena> m_Arenas;
	};
}
