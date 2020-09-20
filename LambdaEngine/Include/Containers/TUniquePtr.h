#pragma once
#include "TUtilities.h"

namespace LambdaEngine
{
	/*
	* TUniquePtr - SmartPointer for scalar types
	*/
	template<typename T>
	class TUniquePtr
	{
	public:
		template<typename TOther>
		friend class TUniquePtr;

		TUniquePtr(const TUniquePtr& other) = delete;
		TUniquePtr& operator=(const TUniquePtr& other) noexcept = delete;

		FORCEINLINE TUniquePtr() noexcept
			: m_pPtr(nullptr)
		{
		}

		FORCEINLINE TUniquePtr(T* pPtr) noexcept
			: m_pPtr(pPtr)
		{
		}

		FORCEINLINE TUniquePtr(TUniquePtr&& other) noexcept
			: m_pPtr(other.m_pPtr)
		{
			other.m_pPtr = nullptr;
		}

		template<typename TOther>
		FORCEINLINE TUniquePtr(TUniquePtr<TOther>&& other) noexcept
			: m_pPtr(other.m_pPtr)
		{
			static_assert(std::is_convertible<TOther, T>);
			other.m_pPtr = nullptr;
		}

		FORCEINLINE ~TUniquePtr()
		{
			Reset();
		}

		FORCEINLINE T* Release() noexcept
		{
			T* WeakPtr = m_pPtr;
			m_pPtr = nullptr;
			return WeakPtr;
		}

		FORCEINLINE void Reset() noexcept
		{
			InternalRelease();
			m_pPtr = nullptr;
		}

		FORCEINLINE void Swap(TUniquePtr& other) noexcept
		{
			T* TempPtr = m_pPtr;
			m_pPtr = other.m_pPtr;
			other.m_pPtr = TempPtr;
		}

		FORCEINLINE T* Get() const noexcept
		{
			return m_pPtr;
		}

		FORCEINLINE T* const* GetAddressOf() const noexcept
		{
			return &m_pPtr;
		}

		FORCEINLINE T* operator->() const noexcept
		{
			return Get();
		}

		FORCEINLINE T& operator*() const noexcept
		{
			return (*m_pPtr);
		}

		FORCEINLINE T* const* operator&() const noexcept
		{
			return GetAddressOf();
		}

		FORCEINLINE TUniquePtr& operator=(T* pPtr) noexcept
		{
			if (m_pPtr != pPtr)
			{
				Reset();
				m_pPtr = pPtr;
			}

			return *this;
		}

		FORCEINLINE TUniquePtr& operator=(TUniquePtr&& other) noexcept
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
		FORCEINLINE TUniquePtr& operator=(TUniquePtr<TOther>&& other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>);

			if (this != std::addressof(other))
			{
				Reset();
				m_pPtr = other.m_pPtr;
				other.m_pPtr = nullptr;
			}

			return *this;
		}

		FORCEINLINE TUniquePtr& operator=(std::nullptr_t) noexcept
		{
			Reset();
			return *this;
		}

		FORCEINLINE bool operator==(const TUniquePtr& other) const noexcept
		{
			return (m_pPtr == other.m_pPtr);
		}

		FORCEINLINE bool operator==(T* pPtr) const noexcept
		{
			return (m_pPtr == pPtr);
		}

		FORCEINLINE operator bool() const noexcept
		{
			return (m_pPtr != nullptr);
		}

	private:
		FORCEINLINE void InternalRelease() noexcept
		{
			if (m_pPtr)
			{
				delete m_pPtr;
				m_pPtr = nullptr;
			}
		}

	private:
		T* m_pPtr;
	};

	/*
	* TUniquePtr - SmartPointer for array types
	*/
	template<typename T>
	class TUniquePtr<T[]>
	{
	public:
		template<typename TOther>
		friend class TUniquePtr;

		TUniquePtr(const TUniquePtr& other) = delete;
		TUniquePtr& operator=(const TUniquePtr& other) noexcept = delete;

		FORCEINLINE TUniquePtr() noexcept
			: m_pPtr(nullptr)
		{
		}

		FORCEINLINE TUniquePtr(T* pPtr) noexcept
			: m_pPtr(pPtr)
		{
		}

		FORCEINLINE TUniquePtr(TUniquePtr&& other) noexcept
			: m_pPtr(other.m_pPtr)
		{
			other.m_pPtr = nullptr;
		}

		template<typename TOther>
		FORCEINLINE TUniquePtr(TUniquePtr<TOther>&& other) noexcept
			: m_pPtr(other.m_pPtr)
		{
			static_assert(std::is_convertible<TOther, T>);
			other.m_pPtr = nullptr;
		}

		FORCEINLINE ~TUniquePtr()
		{
			Reset();
		}

		FORCEINLINE T* Release() noexcept
		{
			T* WeakPtr = m_pPtr;
			m_pPtr = nullptr;
			return WeakPtr;
		}

		FORCEINLINE void Reset() noexcept
		{
			InternalRelease();
			m_pPtr = nullptr;
		}

		FORCEINLINE void Swap(TUniquePtr& other) noexcept
		{
			T* TempPtr = m_pPtr;
			m_pPtr = other.m_pPtr;
			other.m_pPtr = TempPtr;
		}

		FORCEINLINE T* Get() const noexcept
		{
			return m_pPtr;
		}

		FORCEINLINE T* const* GetAddressOf() const noexcept
		{
			return &m_pPtr;
		}

		FORCEINLINE T* const* operator&() const noexcept
		{
			return GetAddressOf();
		}

		FORCEINLINE T& operator[](uint32 index) noexcept
		{
			VALIDATE(m_pPtr != nullptr);
			return m_pPtr[index];
		}

		FORCEINLINE TUniquePtr& operator=(T* pPtr) noexcept
		{
			if (m_pPtr != pPtr)
			{
				Reset();
				m_pPtr = pPtr;
			}

			return *this;
		}

		FORCEINLINE TUniquePtr& operator=(TUniquePtr&& other) noexcept
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
		FORCEINLINE TUniquePtr& operator=(TUniquePtr<TOther>&& other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>);

			if (this != std::addressof(other))
			{
				Reset();
				m_pPtr = other.m_pPtr;
				other.m_pPtr = nullptr;
			}

			return *this;
		}

		FORCEINLINE TUniquePtr& operator=(std::nullptr_t) noexcept
		{
			Reset();
			return *this;
		}

		FORCEINLINE bool operator==(const TUniquePtr& other) const noexcept
		{
			return (m_pPtr == other.m_pPtr);
		}

		FORCEINLINE bool operator==(T* pPtr) const noexcept
		{
			return (m_pPtr == pPtr);
		}

		FORCEINLINE operator bool() const noexcept
		{
			return (m_pPtr != nullptr);
		}

	private:
		FORCEINLINE void InternalRelease() noexcept
		{
			if (m_pPtr)
			{
				delete[] m_pPtr;
				m_pPtr = nullptr;
			}
		}

	private:
		T* m_pPtr;
	};

	/*
	* Creates a new object together with a UniquePtr
	*/
	template<typename T, typename... TArgs>
	std::enable_if_t<!std::is_array_v<T>, TUniquePtr<T>> MakeUnique(TArgs&&... args) noexcept
	{
		T* pUniquePtr = DBG_NEW T(Forward<TArgs>(args)...);
		return Move(TUniquePtr<T>(pUniquePtr));
	}

	/*
	* Creates a new object together with a SharedPtr
	*/
	template<typename T>
	std::enable_if_t<std::is_array_v<T>, TUniquePtr<T>> MakeUnique(uint32 size) noexcept
	{
		using TType = TRemoveExtent<T>;

		TType* pUniquePtr = DBG_NEW TType[size];
		return Move(TUniquePtr<T>(pUniquePtr));
	}
}