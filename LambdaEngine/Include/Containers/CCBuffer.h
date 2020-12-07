#pragma once

#include "Types.h"

//Minimalistic Constant Circular Buffer

namespace LambdaEngine
{
	template<class T, uint32 N>
	class CCBuffer
	{
	public:
		bool HasNewData() const
		{
			return m_ReadHead != m_WriteHead;
		}

		bool Read(T& value)
		{
			value = m_Buffer[m_ReadHead];
			if (!HasNewData())
				return false;

			m_ReadHead = (m_ReadHead + 1) % N;
			return true;
		}		

		void Write(const T& value)
		{
			m_Buffer[m_WriteHead++] = value;
			m_WriteHead %= N;
		}

		void Clear()
		{
			m_WriteHead = 0;
			m_ReadHead = 0;
		}

	private:
		T m_Buffer[N];
		uint32 m_WriteHead = 0;
		uint32 m_ReadHead = 0;
	};
}