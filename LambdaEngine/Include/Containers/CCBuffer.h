#pragma once

#include "Types.h"

//Minimalistic Constant Cicular Buffer

namespace LambdaEngine
{
	template<class T, uint32 N>
	class CCBuffer
	{
	public:
		T& Read()
		{
			T& value = m_Buffer[m_ReadHead++];
			m_ReadHead %= N;
			return value;
		}

		T& Peek()
		{
			return m_Buffer[m_ReadHead];
		}

		void Write(const T& value)
		{
			m_Buffer[m_WriteHead++] = value;
			m_WriteHead %= N;
		}

		void WriteAt(const T& value, uint32 index)
		{
			m_Buffer[index % N] = value;
		}

	private:
		T m_Buffer[N];
		uint32 m_WriteHead = 0;
		uint32 m_ReadHead = 0;
	};
}