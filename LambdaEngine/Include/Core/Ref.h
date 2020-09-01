#pragma once
#include "RefCountedObject.h"

namespace LambdaEngine
{
	template<typename TRefCountedObject>
	class TSharedRef
	{
	public:
		inline TSharedRef(TRefCountedObject* pRefObject = nullptr)
			: m_pRefObject(pRefObject)
		{
		}

		// No except since move should not throw
		inline TSharedRef(TSharedRef&& other) noexcept
			: m_pRefObject(other.m_pRefObject)
		{
			other.m_pRefObject = nullptr;
		}

		inline TSharedRef(const TSharedRef& other)
			: m_pRefObject(other.m_pRefObject)
		{
			if (m_pRefObject)
			{
				m_pRefObject->AddRef();
			}
		}

		inline ~TSharedRef()
		{
			Reset();
		}

		inline void Reset()
		{
			SAFERELEASE(m_pRefObject);
		}

		inline void Assign(TRefCountedObject* pPtr)
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

		inline TRefCountedObject** GetAddress()
		{
			return &m_pRefObject;
		}

		inline const TRefCountedObject* const * GetAddress() const
		{
			return &m_pRefObject;
		}

		inline TRefCountedObject* GetAndAddRef()
		{
			m_pRefObject->AddRef();
			return m_pRefObject;
		}

		inline TRefCountedObject* Get()
		{
			return m_pRefObject;
		}

		inline const TRefCountedObject* Get() const
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

		inline TSharedRef& operator=(TRefCountedObject* pPtr)
		{
			if (m_pRefObject != pPtr)
			{
				Assign(pPtr);
			}

			return *this;
		}

		inline TSharedRef& operator=(TSharedRef&& other)
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

		inline TSharedRef& operator=(const TSharedRef& other)
		{
			if (this != &other)
			{
				Assign(other.m_pRefObject);
			}

			return *this;
		}

		inline bool operator==(const TSharedRef& other) const
		{
			return (m_pRefObject == other.m_pRefObject);
		}

		inline bool operator==(TRefCountedObject* pOther) const
		{
			return (m_pRefObject == pOther);
		}

		inline bool operator==(std::nullptr_t null) const
		{
			return (m_pRefObject == null);
		}

		inline bool operator!=(const TSharedRef& other) const
		{
			return (m_pRefObject != other.m_pRefObject);
		}

		inline bool operator!=(TRefCountedObject* pOther) const
		{
			return (m_pRefObject != pOther);
		}

		inline bool operator!=(std::nullptr_t null) const
		{
			return (m_pRefObject != null);
		}

		inline TRefCountedObject* operator->()
		{
			return Get();
		}

		inline const TRefCountedObject* operator->() const
		{
			return Get();
		}

		inline TRefCountedObject** operator&()
		{
			return GetAddress();
		}

		inline operator bool() const
		{
			return (m_pRefObject != nullptr);
		}

	private:
		TRefCountedObject* m_pRefObject = nullptr;
	};
}