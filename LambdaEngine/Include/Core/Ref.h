#pragma once
#include "RefCountedObject.h"

namespace LambdaEngine
{
	template<typename InterfaceType>
	class Ref
	{
	public:
		inline Ref(InterfaceType* pRefObject = nullptr)
			: m_pRefObject(pRefObject)
		{
		}

		// No except since move should not throw
		inline Ref(Ref&& other) noexcept
			: m_pRefObject(other.m_pRefObject)
		{
			other.m_pRefObject = nullptr;
		}

		inline Ref(const Ref& other)
			: m_pRefObject(other.m_pRefObject)
		{
			if (m_pRefObject)
			{
				m_pRefObject->AddRef();
			}
		}

		inline ~Ref()
		{
			Reset();
		}

		inline void Reset()
		{
			SAFERELEASE(m_pRefObject);
		}

		inline void Assign(InterfaceType* pPtr)
		{
			if (m_pRefObject)
			{
				m_pRefObject->Release();
			}

			m_pRefObject = pPtr;
			if (m_pRefObject)
			{
				m_pRefObject->AddRef();
			}
		}

		inline InterfaceType** GetAddress()
		{
			return &m_pRefObject;
		}

		inline const InterfaceType* const * GetAddress() const
		{
			return &m_pRefObject;
		}

		inline InterfaceType* GetAndAddRef()
		{
			m_pRefObject->AddRef();
			return m_pRefObject;
		}

		inline InterfaceType* Get()
		{
			return m_pRefObject;
		}

		inline const InterfaceType* Get() const
		{
			return m_pRefObject;
		}

		template<typename CastType>
		inline CastType* GetAs()
		{
			return reinterpret_cast<CastType*>(m_pRefObject);
		}

		template<typename CastType>
		inline const CastType* GetAs() const
		{
			return reinterpret_cast<const CastType*>(m_pRefObject);
		}

		inline Ref& operator=(InterfaceType* pPtr)
		{
			if (m_pRefObject != pPtr)
			{
				Assign(pPtr);
			}

			return *this;
		}

		inline Ref& operator=(Ref&& other)
		{
			if (this != other)
			{
				if (m_pRefObject)
				{
					m_pRefObject->Release();
				}

				m_pRefObject = other.m_pRefObject;
				other.m_pRefObject = nullptr;
			}

			return *this;
		}

		inline Ref& operator=(const Ref& other)
		{
			if (this != &other)
			{
				Assign(other.m_pRefObject);
			}

			return *this;
		}

		inline bool operator==(const Ref& other) const
		{
			return (m_pRefObject == other.m_pRefObject);
		}

		inline bool operator==(InterfaceType* pOther) const
		{
			return (m_pRefObject == pOther);
		}

		inline bool operator==(std::nullptr_t null) const
		{
			return (m_pRefObject == null);
		}

		inline bool operator!=(const Ref& other) const
		{
			return (m_pRefObject != other.m_pRefObject);
		}

		inline bool operator!=(InterfaceType* pOther) const
		{
			return (m_pRefObject != pOther);
		}

		inline bool operator!=(std::nullptr_t null) const
		{
			return (m_pRefObject != null);
		}

		inline InterfaceType* operator->()
		{
			return Get();
		}

		inline const InterfaceType* operator->() const
		{
			return Get();
		}

		inline InterfaceType** operator&()
		{
			return GetAddress();
		}

		inline operator bool() const
		{
			return (m_pRefObject != nullptr);
		}

	private:
		InterfaceType* m_pRefObject = nullptr;
	};
}