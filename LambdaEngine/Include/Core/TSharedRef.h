#pragma once
#include "RefCountedObject.h"

namespace LambdaEngine
{
	/*
	* TSharedRef
	*/
	template<typename TRefCountedObject>
	class TSharedRef
	{
	public:
		FORCEINLINE TSharedRef(TRefCountedObject* pRefObject = nullptr)
			: m_pRefObject(pRefObject)
		{
		}

		// No except since move should not throw
		FORCEINLINE TSharedRef(TSharedRef&& other) noexcept
			: m_pRefObject(other.m_pRefObject)
		{
			other.m_pRefObject = nullptr;
		}

		FORCEINLINE TSharedRef(const TSharedRef& other)
			: m_pRefObject(other.m_pRefObject)
		{
			if (m_pRefObject)
			{
				m_pRefObject->AddRef();
			}
		}

		FORCEINLINE ~TSharedRef()
		{
			Reset();
		}

		FORCEINLINE void Reset()
		{
			SAFERELEASE(m_pRefObject);
		}

		FORCEINLINE void Assign(TRefCountedObject* pPtr)
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

		FORCEINLINE TRefCountedObject** GetAddress()
		{
			return &m_pRefObject;
		}

		FORCEINLINE const TRefCountedObject* const * GetAddress() const
		{
			return &m_pRefObject;
		}

		FORCEINLINE TRefCountedObject* GetAndAddRef()
		{
			m_pRefObject->AddRef();
			return m_pRefObject;
		}

		FORCEINLINE TRefCountedObject* Get()
		{
			return m_pRefObject;
		}

		FORCEINLINE const TRefCountedObject* Get() const
		{
			return m_pRefObject;
		}

		template<typename CastType>
		FORCEINLINE CastType* GetAs()
		{
			return reinterpret_cast<CastType*>(m_pRefObject);
		}

		template<typename CastType>
		FORCEINLINE const CastType* GetAs() const
		{
			return reinterpret_cast<const CastType*>(m_pRefObject);
		}

		FORCEINLINE TSharedRef& operator=(TRefCountedObject* pPtr)
		{
			if (m_pRefObject != pPtr)
			{
				Assign(pPtr);
			}

			return *this;
		}

		FORCEINLINE TSharedRef& operator=(TSharedRef&& other)
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

		FORCEINLINE TSharedRef& operator=(const TSharedRef& other)
		{
			if (this != &other)
			{
				Assign(other.m_pRefObject);
			}

			return *this;
		}

		FORCEINLINE bool operator==(const TSharedRef& other) const
		{
			return (m_pRefObject == other.m_pRefObject);
		}

		FORCEINLINE bool operator==(TRefCountedObject* pOther) const
		{
			return (m_pRefObject == pOther);
		}

		FORCEINLINE bool operator==(std::nullptr_t null) const
		{
			return (m_pRefObject == null);
		}

		FORCEINLINE bool operator!=(const TSharedRef& other) const
		{
			return (m_pRefObject != other.m_pRefObject);
		}

		FORCEINLINE bool operator!=(TRefCountedObject* pOther) const
		{
			return (m_pRefObject != pOther);
		}

		FORCEINLINE bool operator!=(std::nullptr_t null) const
		{
			return (m_pRefObject != null);
		}

		FORCEINLINE TRefCountedObject* operator->()
		{
			return Get();
		}

		FORCEINLINE const TRefCountedObject* operator->() const
		{
			return Get();
		}

		FORCEINLINE TRefCountedObject** operator&()
		{
			return GetAddress();
		}

		FORCEINLINE operator bool() const
		{
			return (m_pRefObject != nullptr);
		}

	private:
		TRefCountedObject* m_pRefObject = nullptr;
	};
}