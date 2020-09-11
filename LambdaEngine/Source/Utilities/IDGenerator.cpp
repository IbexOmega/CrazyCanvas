#include "Utilities/IDGenerator.h"

namespace LambdaEngine
{
    IDGenerator::IDGenerator()
        :m_NextFree(0)
    {}

    uint32 IDGenerator::GenID()
    {
        if (!m_Recycled.empty())
        {
            uint32 newID = m_Recycled.front();
            m_Recycled.pop();

            return newID;
        }

        return m_NextFree++;
    }

    void IDGenerator::PopID(uint32 ID)
    {
        m_Recycled.push(ID);
    }
}
