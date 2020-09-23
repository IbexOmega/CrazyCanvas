#pragma once

#include "Containers/IDContainer.h"
#include "Containers/TArray.h"
#include "Containers/THashTable.h"

#include <queue>

namespace LambdaEngine
{
	/*
		Extends a vector to be able to:
		* Use IDs to index elements
		* Pop elements anywhere in the array without breaking ID-index relation

		IDD: ID Data
	*/

	template <typename T>
	class IDDVector : public IDContainer
	{
	public:
		IDDVector() = default;
		~IDDVector() = default;

		// Index vector directly
		T operator[](uint32 index) const
		{
			return m_Data[index];
		}

		T& operator[](uint32 index)
		{
			return m_Data[index];
		}

		// Index vector using ID, assumes ID is linked to an element
		const T& IndexID(uint32 ID) const
		{
			auto indexItr = m_IDToIndex.find(ID);
			VALIDATE_MSG(indexItr != m_IDToIndex.end(), "Attempted to index using an unregistered ID: %d", ID);

			return m_Data[indexItr->second];
		}

		T& IndexID(uint32 ID)
		{
			auto indexItr = m_IDToIndex.find(ID);
			VALIDATE_MSG(indexItr != m_IDToIndex.end(), "Attempted to index using an unregistered ID: %d", ID);

			return m_Data[indexItr->second];
		}

		void PushBack(const T& newElement, uint32 ID)
		{
			m_Data.PushBack(newElement);
			m_IDs.PushBack(ID);
			m_IDToIndex[ID] = m_Data.GetSize() - 1;
		}

		void Pop(uint32 ID) override final
		{
			auto popIndexItr = m_IDToIndex.find(ID);
			VALIDATE_MSG(popIndexItr != m_IDToIndex.end(), "Attempted to pop a non-existing element, ID: %d", ID);

			m_Data[popIndexItr->second] = m_Data.GetBack();
			m_IDs[popIndexItr->second] = m_IDs.GetBack();

			m_IDToIndex[m_IDs.GetBack()] = popIndexItr->second;

			m_Data.PopBack();
			m_IDs.PopBack();

			m_IDToIndex.erase(popIndexItr);
		}

		void Clear()
		{
			m_Data.Clear();
			m_IDs.Clear();
			m_IDToIndex.clear();
		}

		bool HasElement(uint32 ID) const override final
		{
			return m_IDToIndex.contains(ID);
		}

		uint32 Size() const override final
		{
			return m_Data.GetSize();
		}

		bool Empty() const
		{
			return m_Data.IsEmpty();
		}

		TArray<T>& GetVec()
		{
			return m_Data;
		}

		const TArray<T>& GetVec() const
		{
			return m_Data;
		}

		const TArray<uint32>& GetIDs() const override final
		{
			return m_IDs;
		}

		T& Back()
		{
			return m_Data.GetBack();
		}

		typename TArray<T>::Iterator begin() noexcept
		{
			return m_Data.begin();
		}

		typename TArray<T>::Iterator end() noexcept
		{
			return m_Data.end();
		}

		typename TArray<T>::ConstIterator begin() const noexcept
		{
			return m_Data.begin();
		}

		typename TArray<T>::ConstIterator end() const noexcept
		{
			return m_Data.end();
		}

	private:
		TArray<T> m_Data;
		// The ID for each data element. Stored separately from the main data for cache-friendliness.
		TArray<uint32> m_IDs;

		// Maps IDs to indices to the data array
		THashTable<uint32, uint32> m_IDToIndex;
	};

	class IDVector : public IDContainer
	{
	public:
		IDVector() = default;
		~IDVector() = default;

		uint32 operator[](uint32 index) const
		{
			return m_IDs[index];
		}

		void PushBack(uint32 ID)
		{
			m_IDs.PushBack(ID);
			m_IDToIndex.insert({ID, m_IDs.GetSize() - 1});
		}

		void Pop(uint32 ID) override final
		{
			auto popIndexItr = m_IDToIndex.find(ID);
			VALIDATE_MSG(popIndexItr != m_IDToIndex.end(), "Attempted to pop a non-existing element, ID: %d", ID);

			m_IDs[popIndexItr->second] = m_IDs.GetBack();
			m_IDToIndex[m_IDs.GetBack()] = popIndexItr->second;
			m_IDs.PopBack();

			m_IDToIndex.erase(popIndexItr);
		}

		void Clear()
		{
			m_IDs.Clear();
			m_IDToIndex.clear();
		}

		bool HasElement(uint32 ID) const override final
		{
			return m_IDToIndex.contains(ID);
		}

		uint32 Size() const override final
		{
			return m_IDs.GetSize();
		}

		bool Empty() const
		{
			return m_IDs.IsEmpty();
		}

		const TArray<uint32>& GetIDs() const override final
		{
			return m_IDs;
		}

		uint32 Back()
		{
			return m_IDs.GetBack();
		}

		typename TArray<uint32>::Iterator begin() noexcept
		{
			return m_IDs.begin();
		}

		typename TArray<uint32>::Iterator end() noexcept
		{
			return m_IDs.end();
		}

		typename TArray<uint32>::ConstIterator begin() const noexcept
		{
			return m_IDs.begin();
		}

		typename TArray<uint32>::ConstIterator end() const noexcept
		{
			return m_IDs.end();
		}

	private:
		TArray<uint32> m_IDs;
		// Maps IDs to indices to the ID array
		THashTable<uint32, uint32> m_IDToIndex;
	};
}
