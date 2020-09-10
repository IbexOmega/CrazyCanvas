#pragma once
#include "RefCountedObject.h"

namespace LambdaEngine
{
	/*
	* TSharedRef - Helper class when using objects with RefCountedObject as a base
	*/
	template<typename TRefCountedObject>
	class TSharedRef
	{
	public:
		template<typename TOther>
		friend class TSharedRef;

		FORCEINLINE TSharedRef() noexcept
			: m_pPtr(nullptr)
		{
			static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
		}

		FORCEINLINE TSharedRef(const TSharedRef& other) noexcept
			: m_pPtr(other.m_pPtr)
		{
			static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
			AddRef();
		}

		template<typename TOther>
		FORCEINLINE TSharedRef(const TSharedRef<TOther>& other) noexcept
			: m_pPtr(other.m_pPtr)
		{
			static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
			static_assert(std::is_base_of<RefCountedObject, TOther>());

			AddRef();
		}

		FORCEINLINE TSharedRef(TSharedRef&& other) noexcept
			: m_pPtr(other.m_pPtr)
		{
			static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
			other.m_pPtr = nullptr;
		}

		template<typename TOther>
		FORCEINLINE TSharedRef(TSharedRef<TOther>&& other) noexcept
			: m_pPtr(other.m_pPtr)
		{
			static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
			static_assert(std::is_base_of<RefCountedObject, TOther>());

			other.m_pPtr = nullptr;
		}

		FORCEINLINE TSharedRef(TRefCountedObject* pPtr) noexcept
			: m_pPtr(pPtr)
		{
			static_assert(std::is_base_of<RefCountedObject, TRefCountedObject>());
		}

		template<typename TOther>
		FORCEINLINE TSharedRef(TOther* pPtr) noexcept
			: m_pPtr(pPtr)
		{
			static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
			static_assert(std::is_base_of<RefCountedObject, TOther>());
		}

		FORCEINLINE ~TSharedRef()
		{
			Reset();
		}

		FORCEINLINE TRefCountedObject* Reset() noexcept
		{
			TRefCountedObject* pWeakPtr = m_pPtr;
			InternalRelease();

			return pWeakPtr;
		}

		FORCEINLINE void AddRef() noexcept
		{
			if (m_pPtr)
			{
				m_pPtr->AddRef();
			}
		}

		FORCEINLINE void Swap(TRefCountedObject* pPtr) noexcept
		{
			Reset();
			m_pPtr = pPtr;
		}

		template<typename TOther>
		FORCEINLINE void Swap(TOther* pPtr) noexcept
		{
			static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
			static_assert(std::is_base_of<RefCountedObject, TOther>());

			Reset();
			m_pPtr = pPtr;
		}

		FORCEINLINE TRefCountedObject* Get() const noexcept
		{
			return m_pPtr;
		}

		FORCEINLINE TRefCountedObject* GetAndAddRef() noexcept
		{
			AddRef();
			return m_pPtr;
		}

		template<typename TCastType>
		FORCEINLINE TCastType* GetAs() const noexcept
		{
			static_assert(std::is_convertible<TCastType*, TRefCountedObject*>());
			return static_cast<TCastType*>(m_pPtr);
		}

		FORCEINLINE TRefCountedObject* const* GetAddressOf() const noexcept
		{
			return &m_pPtr;
		}

		FORCEINLINE TRefCountedObject* operator->() const noexcept
		{
			return Get();
		}

		FORCEINLINE TRefCountedObject* const* operator&() const noexcept
		{
			return GetAddressOf();
		}

		FORCEINLINE bool operator==(TRefCountedObject* pPtr) const noexcept
		{
			return (m_pPtr == pPtr);
		}

		FORCEINLINE bool operator==(const TSharedRef& other) const noexcept
		{
			return (m_pPtr == other.m_pPtr);
		}

		FORCEINLINE bool operator!=(TRefCountedObject* pPtr) const noexcept
		{
			return (m_pPtr != pPtr);
		}

		FORCEINLINE bool operator!=(const TSharedRef& other) const noexcept
		{
			return (m_pPtr != other.m_pPtr);
		}

		FORCEINLINE bool operator==(std::nullptr_t) const noexcept
		{
			return (m_pPtr == nullptr);
		}

		FORCEINLINE bool operator!=(std::nullptr_t) const noexcept
		{
			return (m_pPtr != nullptr);
		}

		template<typename TOther>
		FORCEINLINE bool operator==(TOther* pPtr) const noexcept
		{
			static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
			static_assert(std::is_base_of<RefCountedObject, TOther>());

			return (m_pPtr == pPtr);
		}

		template<typename TOther>
		FORCEINLINE bool operator==(const TSharedRef<TOther>& other) const noexcept
		{
			static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
			static_assert(std::is_base_of<RefCountedObject, TOther>());

			return (m_pPtr == other.m_pPtr);
		}

		template<typename TOther>
		FORCEINLINE bool operator!=(TOther* pPtr) const noexcept
		{
			static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
			static_assert(std::is_base_of<RefCountedObject, TOther>());

			return (m_pPtr != pPtr);
		}

		template<typename TOther>
		FORCEINLINE bool operator!=(const TSharedRef<TOther>& other) const noexcept
		{
			static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
			static_assert(std::is_base_of<RefCountedObject, TOther>());

			return (m_pPtr != other.m_pPtr);
		}

		FORCEINLINE operator bool() const noexcept
		{
			return (m_pPtr != nullptr);
		}

		FORCEINLINE TSharedRef& operator=(const TSharedRef& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();

				m_pPtr = other.m_pPtr;
				AddRef();
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TSharedRef& operator=(const TSharedRef<TOther>& other) noexcept
		{
			static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
			static_assert(std::is_base_of<RefCountedObject, TOther>());

			if (this != std::addressof(other))
			{
				Reset();

				m_pPtr = other.m_pPtr;
				AddRef();
			}

			return *this;
		}

		FORCEINLINE TSharedRef& operator=(TSharedRef&& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();

				m_pPtr = other.m_pPtr;
				other.m_pPtr = nullptr;
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TSharedRef& operator=(TSharedRef<TOther>&& other) noexcept
		{
			static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
			static_assert(std::is_base_of<RefCountedObject, TOther>());

			if (this != std::addressof(other))
			{
				Reset();

				m_pPtr = other.m_pPtr;
				other.m_pPtr = nullptr;
			}

			return *this;
		}

		FORCEINLINE TSharedRef& operator=(TRefCountedObject* pPtr) noexcept
		{
			if (m_pPtr != pPtr)
			{
				Reset();
				m_pPtr = pPtr;
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TSharedRef& operator=(TOther* pPtr) noexcept
		{
			static_assert(std::is_convertible<TOther*, TRefCountedObject*>());
			static_assert(std::is_base_of<RefCountedObject, TOther>());

			if (m_pPtr != pPtr)
			{
				Reset();
				m_pPtr = pPtr;
			}

			return *this;
		}

		FORCEINLINE TSharedRef& operator=(std::nullptr_t) noexcept
		{
			Reset();
			return *this;
		}

	private:
		FORCEINLINE void InternalRelease() noexcept
		{
			if (m_pPtr)
			{
				m_pPtr->Release();
				m_pPtr = nullptr;
			}
		}

	private:
		TRefCountedObject* m_pPtr;
	};

	template<typename TRefCountedObject>
	TSharedRef<TRefCountedObject> MakeSharedRef(TRefCountedObject* pSharedRef)
	{
		pSharedRef->AddRef();
		return Move(TSharedRef<TRefCountedObject>(pSharedRef));
	}
}