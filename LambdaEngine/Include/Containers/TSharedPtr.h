#pragma once
#include "TUniquePtr.h"

namespace LambdaEngine
{
	/*
	* Struct Counting references in TWeak- and TSharedPtr
	*/
	struct PtrControlBlock
	{
	public:
		typedef uint32 RefType;

		FORCEINLINE RefType AddWeakRef() noexcept
		{
			return m_WeakReferences++;
		}

		FORCEINLINE RefType AddStrongRef() noexcept
		{
			return m_StrongReferences++;
		}

		FORCEINLINE RefType ReleaseWeakRef() noexcept
		{
			return m_WeakReferences--;
		}

		FORCEINLINE RefType ReleaseStrongRef() noexcept
		{
			return m_StrongReferences--;
		}

		FORCEINLINE RefType GetWeakReferences() const noexcept
		{
			return m_WeakReferences;
		}

		FORCEINLINE RefType GetStrongReferences() const noexcept
		{
			return m_StrongReferences;
		}

	private:
		RefType m_WeakReferences;
		RefType m_StrongReferences;
	};

	/*
	* Base class for TWeak- and TSharedPtr
	*/
	template<typename T>
	class TPtrBase
	{
	public:
		template<typename TOther>
		friend class TPtrBase;

		FORCEINLINE T* Get() const noexcept
		{
			return m_pPtr;
		}

		FORCEINLINE T* const* GetAddressOf() const noexcept
		{
			return &m_pPtr;
		}

		FORCEINLINE uint32 GetStrongReferences() const noexcept
		{
			return m_pCounter ? m_pCounter->GetStrongReferences() : 0;
		}

		FORCEINLINE uint32 GetWeakReferences() const noexcept
		{
			return m_pCounter ? m_pCounter->GetWeakReferences() : 0;
		}

		FORCEINLINE T* const* operator&() const noexcept
		{
			return GetAddressOf();
		}

		FORCEINLINE T* operator->() const noexcept
		{
			return Get();
		}

		FORCEINLINE T& operator*() const noexcept
		{
			return (*m_pPtr);
		}

		FORCEINLINE T& operator[](uint32 Index) const noexcept
		{
			VALIDATE(m_pPtr != nullptr);
			return m_pPtr[Index];
		}

		FORCEINLINE bool operator==(T* pPtr) const noexcept
		{
			return (m_pPtr == pPtr);
		}

		FORCEINLINE bool operator!=(T* pPtr) const noexcept
		{
			return (m_pPtr != pPtr);
		}

		FORCEINLINE operator bool() const noexcept
		{
			return (m_pPtr != nullptr);
		}

	protected:
		FORCEINLINE TPtrBase() noexcept
			: m_pPtr(nullptr)
			, m_pCounter(nullptr)
		{
		}

		FORCEINLINE void InternalAddStrongRef() noexcept
		{
			// If the object has a Ptr there must be a Counter or something went wrong
			if (m_pPtr)
			{
				VALIDATE(m_pCounter != nullptr);
				m_pCounter->AddStrongRef();
			}
		}

		FORCEINLINE void InternalAddWeakRef() noexcept
		{
			// If the object has a Ptr there must be a Counter or something went wrong
			if (m_pPtr)
			{
				VALIDATE(m_pCounter != nullptr);
				m_pCounter->AddWeakRef();
			}
		}

		FORCEINLINE void InternalReleaseStrongRef() noexcept
		{
			// If the object has a Ptr there must be a Counter or something went wrong
			if (m_pPtr)
			{
				VALIDATE(m_pCounter != nullptr);
				m_pCounter->ReleaseStrongRef();

				// When releasing the last strong reference we can destroy the pointer and counter
				if (m_pCounter->GetStrongReferences() <= 0)
				{
					delete m_pPtr;
					delete m_pCounter;
					InternalClear();
				}
			}
		}

		FORCEINLINE void InternalReleaseWeakRef() noexcept
		{
			// If the object has a Ptr there must be a Counter or something went wrong
			if (m_pPtr)
			{
				VALIDATE(m_pCounter != nullptr);
				m_pCounter->ReleaseWeakRef();
			}
		}

		FORCEINLINE void InternalSwap(TPtrBase& other) noexcept
		{
			T* pTempPtr = m_pPtr;
			PtrControlBlock* pTempBlock = m_pCounter;

			m_pPtr = other.m_pPtr;
			m_pCounter = other.m_pCounter;

			other.m_pPtr = pTempPtr;
			other.m_pCounter = pTempBlock;
		}

		FORCEINLINE void InternalMove(TPtrBase&& other) noexcept
		{
			m_pPtr = other.m_pPtr;
			m_pCounter = other.m_pCounter;

			other.m_pPtr = nullptr;
			other.m_pCounter = nullptr;
		}

		template<typename TOther>
		FORCEINLINE void InternalMove(TPtrBase<TOther>&& other) noexcept
		{
			static_assert(std::is_convertible<TOther*, T*>());

			m_pPtr = static_cast<TOther*>(other.m_pPtr);
			m_pCounter = other.m_pCounter;

			other.m_pPtr = nullptr;
			other.m_pCounter = nullptr;
		}

		FORCEINLINE void InternalConstructStrong(T* pPtr)
		{
			m_pPtr = pPtr;
			m_pCounter = new PtrControlBlock();
			InternalAddStrongRef();
		}

		template<typename TOther>
		FORCEINLINE void InternalConstructStrong(TOther* pPtr)
		{
			static_assert(std::is_convertible<TOther*, T*>());

			m_pPtr = static_cast<T*>(pPtr);
			m_pCounter = new PtrControlBlock();
			InternalAddStrongRef();
		}

		FORCEINLINE void InternalConstructStrong(const TPtrBase& other)
		{
			m_pPtr = other.m_pPtr;
			m_pCounter = other.m_pCounter;
			InternalAddStrongRef();
		}

		template<typename TOther>
		FORCEINLINE void InternalConstructStrong(const TPtrBase<TOther>& other)
		{
			static_assert(std::is_convertible<TOther*, T*>());

			m_pPtr = static_cast<T*>(other.m_pPtr);
			m_pCounter = other.m_pCounter;
			InternalAddStrongRef();
		}

		FORCEINLINE void InternalConstructWeak(T* pPtr)
		{
			m_pPtr = pPtr;
			m_pCounter = new PtrControlBlock();
			InternalAddWeakRef();
		}

		template<typename TOther>
		FORCEINLINE void InternalConstructWeak(TOther* pPtr)
		{
			static_assert(std::is_convertible<TOther*, T*>());

			m_pPtr = static_cast<T*>(pPtr);
			m_pCounter = new PtrControlBlock();
			InternalAddWeakRef();
		}

		FORCEINLINE void InternalConstructWeak(const TPtrBase& other)
		{
			m_pPtr = other.m_pPtr;
			m_pCounter = other.m_pCounter;
			InternalAddWeakRef();
		}

		template<typename TOther>
		FORCEINLINE void InternalConstructWeak(const TPtrBase<TOther>& other)
		{
			static_assert(std::is_convertible<TOther*, T*>());

			m_pPtr = static_cast<T*>(other.m_pPtr);
			m_pCounter = other.m_pCounter;
			InternalAddWeakRef();
		}

		FORCEINLINE void InternalDestructWeak()
		{
			InternalReleaseWeakRef();
			InternalClear();
		}

		FORCEINLINE void InternalDestructStrong()
		{
			InternalReleaseStrongRef();
			InternalClear();
		}

		FORCEINLINE void InternalClear() noexcept
		{
			m_pPtr = nullptr;
			m_pCounter = nullptr;
		}

		T* m_pPtr;
		PtrControlBlock* m_pCounter;
	};

	/*
	* Forward Declarations
	*/
	template<typename TOther>
	class TWeakPtr;

	/*
	* TSharedPtr - RefCounted Pointer similar to std::shared_ptr
	*/
	template<typename T>
	class TSharedPtr : public TPtrBase<T>
	{
		using TBase = TPtrBase<T>;

	public:
		FORCEINLINE TSharedPtr() noexcept
			: TBase()
		{
		}

		FORCEINLINE TSharedPtr(std::nullptr_t) noexcept
			: TBase()
		{
		}

		FORCEINLINE explicit TSharedPtr(T* pPtr) noexcept
			: TBase()
		{
			TBase::InternalConstructStrong(pPtr);
		}

		FORCEINLINE TSharedPtr(const TSharedPtr& other) noexcept
			: TBase()
		{
			TBase::InternalConstructStrong(other);
		}

		FORCEINLINE TSharedPtr(TSharedPtr&& other) noexcept
			: TBase()
		{
			TBase::InternalMove(Move(other));
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr(const TSharedPtr<TOther>& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther*, T*>());
			TBase::template InternalConstructStrong<TOther>(other);
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr(TSharedPtr<TOther>&& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther*, T*>());
			TBase::template InternalMove<TOther>(Move(other));
		}

		template<typename TOther>
		FORCEINLINE explicit TSharedPtr(const TWeakPtr<TOther>& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther*, T*>());
			TBase::template InternalConstructStrong<TOther>(other);
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr(TUniquePtr<TOther>&& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther*, T*>());
			TBase::template InternalConstructStrong<TOther>(other.Release());
		}

		FORCEINLINE ~TSharedPtr()
		{
			Reset();
		}

		FORCEINLINE void Reset() noexcept
		{
			TBase::InternalDestructStrong();
		}

		FORCEINLINE void Swap(TSharedPtr& other) noexcept
		{
			TBase::InternalSwap(other);
		}

		FORCEINLINE bool IsUnique() const noexcept
		{
			return (TBase::GetStrongReferences() == 1);
		}

		FORCEINLINE TSharedPtr& operator=(const TSharedPtr& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalConstructStrong(other);
			}

			return *this;
		}

		FORCEINLINE TSharedPtr& operator=(TSharedPtr&& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalMove(Move(other));
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr& operator=(const TSharedPtr<TOther>& other) noexcept
		{
			static_assert(std::is_convertible<TOther*, T*>());

			if (this != std::addressof(other))
			{
				Reset();
				TBase::template InternalConstructStrong<TOther>(other);
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr& operator=(TSharedPtr<TOther>&& other) noexcept
		{
			static_assert(std::is_convertible<TOther*, T*>());

			if (this != std::addressof(other))
			{
				Reset();
				TBase::template InternalMove<TOther>(Move(other));
			}

			return *this;
		}

		FORCEINLINE TSharedPtr& operator=(T* pPtr) noexcept
		{
			if (this->m_pPtr != pPtr)
			{
				Reset();
				TBase::InternalConstructStrong(pPtr);
			}

			return *this;
		}

		FORCEINLINE TSharedPtr& operator=(std::nullptr_t) noexcept
		{
			Reset();
			return *this;
		}

		FORCEINLINE bool operator==(const TSharedPtr& other) const noexcept
		{
			return (TBase::m_pPtr == other.m_pPtr);
		}

		FORCEINLINE bool operator!=(const TSharedPtr& other) const noexcept
		{
			return (TBase::m_pPtr != other.m_pPtr);
		}

		FORCEINLINE bool operator==(TSharedPtr&& other) const noexcept
		{
			return (TBase::m_pPtr == other.m_pPtr);
		}

		FORCEINLINE bool operator!=(TSharedPtr&& other) const noexcept
		{
			return (TBase::m_pPtr != other.m_pPtr);
		}
	};

	/*
	* TWeakPtr - Weak Pointer similar to std::weak_ptr
	*/
	template<typename T>
	class TWeakPtr : public TPtrBase<T>
	{
		using TBase = TPtrBase<T>;

	public:
		FORCEINLINE TWeakPtr() noexcept
			: TBase()
		{
		}

		FORCEINLINE TWeakPtr(const TSharedPtr<T>& pPtr) noexcept
			: TBase()
		{
			TBase::InternalConstructWeak(pPtr);
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr(const TSharedPtr<TOther>& pPtr) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
			TBase::template InternalConstructWeak<TOther>(pPtr);
		}

		FORCEINLINE TWeakPtr(const TWeakPtr& other) noexcept
			: TBase()
		{
			TBase::InternalConstructWeak(other);
		}

		FORCEINLINE TWeakPtr(TWeakPtr&& other) noexcept
			: TBase()
		{
			TBase::InternalMove(Move(other));
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr(const TWeakPtr<TOther>& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
			TBase::template InternalConstructWeak<TOther>(other);
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr(TWeakPtr<TOther>&& other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
			TBase::template InternalMove<TOther>(Move(other));
		}

		FORCEINLINE ~TWeakPtr()
		{
			Reset();
		}

		FORCEINLINE void Reset() noexcept
		{
			TBase::InternalDestructWeak();
		}

		FORCEINLINE void Swap(TWeakPtr& other) noexcept
		{
			TBase::InternalSwap(other);
		}

		FORCEINLINE bool IsExpired() const noexcept
		{
			return (TBase::GetStrongReferences() < 1);
		}

		FORCEINLINE TSharedPtr<T> MakeShared() noexcept
		{
			const TWeakPtr& This = *this;
			return Move(TSharedPtr<T>(This));
		}

		FORCEINLINE TWeakPtr& operator=(const TWeakPtr& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalConstructWeak(other);
			}

			return *this;
		}

		FORCEINLINE TWeakPtr& operator=(TWeakPtr&& other) noexcept
		{
			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalMove(Move(other));
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr& operator=(const TWeakPtr<TOther>& other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");

			if (this != std::addressof(other))
			{
				Reset();
				TBase::template InternalConstructWeak<TOther>(other);
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr& operator=(TWeakPtr<TOther>&& other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");

			if (this != std::addressof(other))
			{
				Reset();
				TBase::InternalMove(Move(other));
			}

			return *this;
		}

		FORCEINLINE TWeakPtr& operator=(T* pPtr) noexcept
		{
			if (TBase::m_pPtr != pPtr)
			{
				Reset();
				TBase::InternalConstructWeak(pPtr);
			}

			return *this;
		}

		FORCEINLINE TWeakPtr& operator=(std::nullptr_t) noexcept
		{
			Reset();
			return *this;
		}

		FORCEINLINE bool operator==(const TWeakPtr& other) const noexcept
		{
			return (TBase::m_pPtr == other.m_pPtr);
		}

		FORCEINLINE bool operator!=(const TWeakPtr& other) const noexcept
		{
			return (TBase::m_pPtr != other.m_pPtr);
		}

		FORCEINLINE bool operator==(TWeakPtr&& other) const noexcept
		{
			return (TBase::m_pPtr == other.m_pPtr);
		}

		FORCEINLINE bool operator!=(TWeakPtr&& other) const noexcept
		{
			return (TBase::m_pPtr != other.m_pPtr);
		}
	};

	/*
	* Creates a new object together with a SharedPtr
	*/
	template<typename T, typename... TArgs>
	TSharedPtr<T> MakeShared(TArgs&&... args) noexcept
	{
		T* pRefCountedPtr = DBG_NEW T(Forward<TArgs>(args)...);
		return Move(TSharedPtr<T>(pRefCountedPtr));
	}
}