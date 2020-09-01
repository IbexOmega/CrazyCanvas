#pragma once
#include "TUtilities.h"

namespace LambdaEngine
{
	/*
	* TUniquePtr - SmartPointer similar to std::unique_ptr
	*/
	template<typename T>
	class TUniquePtr
	{
	public:
		template<typename TOther>
		friend class TUniquePtr;

		TUniquePtr(const TUniquePtr& Other) = delete;
		TUniquePtr& operator=(const TUniquePtr& Other) noexcept = delete;

		FORCEINLINE TUniquePtr() noexcept
			: m_Ptr(nullptr)
		{
		}

		FORCEINLINE TUniquePtr(T* InPtr) noexcept
			: m_Ptr(InPtr)
		{
		}

		FORCEINLINE TUniquePtr(TUniquePtr&& Other) noexcept
			: m_Ptr(Other.m_Ptr)
		{
			Other.m_Ptr = nullptr;
		}

		template<typename TOther>
		FORCEINLINE TUniquePtr(TUniquePtr<TOther>&& Other) noexcept
			: m_Ptr(Other.m_Ptr)
		{
			Other.m_Ptr = nullptr;
		}

		FORCEINLINE ~TUniquePtr()
		{
			Reset();
		}

		FORCEINLINE T* Release() noexcept
		{
			T* WeakPtr = m_Ptr;
			m_Ptr = nullptr;
			return WeakPtr;
		}

		FORCEINLINE void Reset() noexcept
		{
			InternalRelease();
			m_Ptr = nullptr;
		}

		FORCEINLINE void Swap(TUniquePtr& Other) noexcept
		{
			T* TempPtr = m_Ptr;
			m_Ptr = Other.m_Ptr;
			Other.m_Ptr = TempPtr;
		}

		FORCEINLINE T* Get() const noexcept
		{
			return m_Ptr;
		}

		FORCEINLINE T* const* GetAddressOf() const noexcept
		{
			return &m_Ptr;
		}

		FORCEINLINE T* operator->() const noexcept
		{
			return Get();
		}

		FORCEINLINE T& operator*() const noexcept
		{
			return (*m_Ptr);
		}

		FORCEINLINE T* const* operator&() const noexcept
		{
			return GetAddressOf();
		}

		FORCEINLINE T& operator[](Uint32 Index) noexcept
		{
			VALIDATE(m_Ptr != nullptr);
			return m_Ptr[Index];
		}

		FORCEINLINE TUniquePtr& operator=(T* InPtr) noexcept
		{
			if (m_Ptr != InPtr)
			{
				Reset();
				m_Ptr = InPtr;
			}

			return *this;
		}

		FORCEINLINE TUniquePtr& operator=(TUniquePtr&& Other) noexcept
		{
			if (this != std::addressof(Other))
			{
				Reset();
				m_Ptr = Other.m_Ptr;
				Other.m_Ptr = nullptr;
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TUniquePtr& operator=(TUniquePtr<TOther>&& Other) noexcept
		{
			if (this != std::addressof(Other))
			{
				Reset();
				m_Ptr = Other.m_Ptr;
				Other.m_Ptr = nullptr;
			}

			return *this;
		}

		FORCEINLINE TUniquePtr& operator=(std::nullptr_t) noexcept
		{
			Reset();
			return *this;
		}

		FORCEINLINE bool operator==(const TUniquePtr& Other) const noexcept
		{
			return (m_Ptr == Other.m_Ptr);
		}

		FORCEINLINE bool operator==(T* InPtr) const noexcept
		{
			return (m_Ptr == InPtr);
		}

		FORCEINLINE operator bool() const noexcept
		{
			return (m_Ptr != nullptr);
		}

	private:
		FORCEINLINE void InternalRelease() noexcept
		{
			if (m_Ptr)
			{
				delete m_Ptr;
				m_Ptr = nullptr;
			}
		}

		T* m_Ptr;
	};

	/*
	* Creates a new object together with a UniquePtr
	*/
	template<typename T, typename... TArgs>
	TUniquePtr<T> MakeUnique(TArgs&&... Args) noexcept
	{
		T* UniquePtr = new T(Forward<TArgs>(Args)...);
		return Move(TUniquePtr<T>(UniquePtr));
	}
}