#pragma once

#include "Containers/IDContainer.h"
#include "Containers/TArray.h"

#include <queue>

namespace LambdaEngine
{
    /*
        Extends a vector to be able to:
        * Use IDs to index elements
        * Pop elements in the middle of the array without breaking ID-index relation

        Stores elements, index for each ID and ID for each element

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
            return m_Data[m_Indices[ID]];
        }

        T& IndexID(uint32 ID)
        {
            return m_Data[m_Indices[ID]];
        }

        void PushBack(const T& newElement, uint32 ID)
        {
            m_Data.PushBack(newElement);
            m_IDs.PushBack(ID);

            // Link element ID to index, resize ID vector in case ID is larger than the vector's size
            if (m_Indices.GetSize() < ID + 1)
            {
                m_Indices.Resize(ID + 1);
            }

            m_Indices[ID] = m_Data.GetSize() - 1;
        }

        void Pop(uint32 ID) override final
        {
            uint32 popIndex = m_Indices[ID];

            if (popIndex < m_Data.GetSize() - 1)
            {
                // Replace to be popped element with the rear element
                m_Data[popIndex] = m_Data.GetBack();
                m_IDs[popIndex] = m_IDs.GetBack();

                m_Indices[m_IDs.GetBack()] = popIndex;
            }

            m_Data.PopBack();
            m_IDs.PopBack();

            if (m_Data.IsEmpty())
            {
                m_Indices.Clear();
            }

            // The rear elements in the indices vector might point at deleted elements, clean them up
            while (m_Indices.GetSize() > m_Data.GetSize() && m_Indices.GetBack() >= m_Data.GetSize())
            {
                m_Indices.PopBack();
            }
        }

        void Clear()
        {
            m_Data.Clear();
            m_IDs.Clear();
            m_Indices.Clear();
        }

        bool HasElement(uint32 ID) const override final
        {
            // Check if the ID pointed at by indices[ID] is the same as the parameter ID
            return ID < m_Indices.GetSize() && ID == m_IDs[m_Indices[ID]];
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
            return this->m_Data;
        }

        const TArray<T>& GetVec() const
        {
            return this->m_Data;
        }

        const TArray<uint32>& GetIDs() const override final
        {
            return this->m_IDs;
        }

        T& Back()
        {
            return this->m_Data.GetBack();
        }

    private:
        TArray<T> m_Data;

        /*
            Stores indices to the main vector, use entity IDs to index it, eg:
            T myElement = vec[indices[ID]];
            or
            uint32 entityIndex = indices[entityID];
        */
        TArray<uint32> m_Indices;

        // Stores index for each ID, eg. ids[5] == vec[5].id (had vec's elements contained IDs)
        TArray<uint32> m_IDs;
    };

    class IDVector : public IDContainer
    {
    public:
        IDVector() {}
        ~IDVector() {}

        uint32 operator[](uint32 index) const
        {
            return m_IDs[index];
        }

        void PushBack(uint32 ID)
        {
            m_IDs.PushBack(ID);

            // Link element ID to index, resize ID vector in case ID is larger than the vector's size
            if (m_Indices.GetSize() < ID + 1)
            {
                m_Indices.Resize(ID + 1);
            }

            m_Indices[ID] = m_Indices.GetSize() - 1;
        }

        void Pop(uint32 ID) override final
        {
            uint32 popIndex = m_Indices[ID];

            if (popIndex < m_IDs.GetSize() - 1)
            {
                // Replace to be popped element with the rear element
                m_IDs[popIndex] = m_IDs.GetBack();

                m_Indices[m_IDs.GetBack()] = popIndex;
            }

            m_IDs.PopBack();

            if (m_IDs.IsEmpty())
            {
                m_Indices.Clear();
            }

            // The rear elements in the indices vector might point at deleted elements, clean them up
            while (m_Indices.GetSize() > m_IDs.GetSize() && m_Indices.GetBack() >= m_IDs.GetSize())
            {
                m_Indices.PopBack();
            }
        }

        void Clear()
        {
            m_IDs.Clear();
            m_Indices.Clear();
        }

        bool HasElement(uint32 ID) const override final
        {
            // Check if the ID pointed at by indices[ID] is the same as the parameter ID
            return ID < m_Indices.GetSize() && ID == m_IDs[m_Indices[ID]];
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

    private:
        /*
            Stores indices to the main vector, use entity IDs to index it, eg:
            T myElement = vec[indices[ID]];
            or
            uint32 entityIndex = indices[entityID];
        */
        TArray<uint32> m_Indices;

        // Stores index for each ID, eg. ids[5] == vec[5].id (had vec's elements contained IDs)
        TArray<uint32> m_IDs;
    };
}

