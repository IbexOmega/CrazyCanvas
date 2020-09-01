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
			return m_Ptr;
		}

		FORCEINLINE T* const* GetAddressOf() const noexcept
		{
			return &m_Ptr;
		}

		FORCEINLINE Uint32 GetStrongReferences() const noexcept
		{
			return m_Counter ? m_Counter->GetStrongReferences() : 0;
		}

		FORCEINLINE Uint32 GetWeakReferences() const noexcept
		{
			return m_Counter ? m_Counter->GetWeakReferences() : 0;
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
			return (*m_Ptr);
		}

		FORCEINLINE T& operator[](Uint32 Index) const noexcept
		{
			VALIDATE(m_Ptr != nullptr);
			return m_Ptr[Index];
		}

		FORCEINLINE bool operator==(T* InPtr) const noexcept
		{
			return (m_Ptr == InPtr);
		}

		FORCEINLINE bool operator!=(T* InPtr) const noexcept
		{
			return (m_Ptr != InPtr);
		}

		FORCEINLINE operator bool() const noexcept
		{
			return (m_Ptr != nullptr);
		}

	protected:
		FORCEINLINE TPtrBase() noexcept
			: m_Ptr(nullptr)
			, m_Counter(nullptr)
		{
		}

		FORCEINLINE void InternalAddStrongRef() noexcept
		{
			// If the object has a Ptr there must be a Counter or something went wrong
			if (m_Ptr)
			{
				VALIDATE(m_Counter != nullptr);
				m_Counter->AddStrongRef();
			}
		}

		FORCEINLINE void InternalAddWeakRef() noexcept
		{
			// If the object has a Ptr there must be a Counter or something went wrong
			if (m_Ptr)
			{
				VALIDATE(m_Counter != nullptr);
				m_Counter->AddWeakRef();
			}
		}

		FORCEINLINE void InternalReleaseStrongRef() noexcept
		{
			// If the object has a Ptr there must be a Counter or something went wrong
			if (m_Ptr)
			{
				VALIDATE(m_Counter != nullptr);
				m_Counter->ReleaseStrongRef();

				// When releasing the last strong reference we can destroy the pointer and counter
				if (m_Counter->GetStrongReferences() <= 0)
				{
					delete m_Ptr;
					delete m_Counter;
					InternalClear();
				}
			}
		}

		FORCEINLINE void InternalReleaseWeakRef() noexcept
		{
			// If the object has a Ptr there must be a Counter or something went wrong
			if (m_Ptr)
			{
				VALIDATE(m_Counter != nullptr);
				m_Counter->ReleaseWeakRef();
			}
		}

		FORCEINLINE void InternalSwap(TPtrBase& Other) noexcept
		{
			T* TempPtr = m_Ptr;
			PtrControlBlock* TempBlock = m_Counter;

			m_Ptr = Other.m_Ptr;
			m_Counter = Other.m_Counter;

			Other.m_Ptr = TempPtr;
			Other.m_Counter = TempBlock;
		}

		FORCEINLINE void InternalMove(TPtrBase&& Other) noexcept
		{
			m_Ptr = Other.m_Ptr;
			m_Counter = Other.m_Counter;

			Other.m_Ptr = nullptr;
			Other.m_Counter = nullptr;
		}

		template<typename TOther>
		FORCEINLINE void InternalMove(TPtrBase<TOther>&& Other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>());

			m_Ptr = static_cast<TOther*>(Other.m_Ptr);
			m_Counter = Other.m_Counter;

			Other.m_Ptr = nullptr;
			Other.m_Counter = nullptr;
		}

		FORCEINLINE void InternalConstructStrong(T* InPtr)
		{
			m_Ptr = InPtr;
			m_Counter = new PtrControlBlock();
			InternalAddStrongRef();
		}

		template<typename TOther>
		FORCEINLINE void InternalConstructStrong(TOther* InPtr)
		{
			static_assert(std::is_convertible<TOther, T>());

			m_Ptr = static_cast<T*>(InPtr);
			m_Counter = new PtrControlBlock();
			InternalAddStrongRef();
		}

		FORCEINLINE void InternalConstructStrong(const TPtrBase& Other)
		{
			m_Ptr = Other.m_Ptr;
			m_Counter = Other.m_Counter;
			InternalAddStrongRef();
		}

		template<typename TOther>
		FORCEINLINE void InternalConstructStrong(const TPtrBase<TOther>& Other)
		{
			static_assert(std::is_convertible<TOther, T>());

			m_Ptr = static_cast<T*>(Other.m_Ptr);
			m_Counter = Other.m_Counter;
			InternalAddStrongRef();
		}

		FORCEINLINE void InternalConstructWeak(T* InPtr)
		{
			m_Ptr = InPtr;
			m_Counter = new PtrControlBlock();
			InternalAddWeakRef();
		}

		template<typename TOther>
		FORCEINLINE void InternalConstructWeak(TOther* InPtr)
		{
			static_assert(std::is_convertible<TOther, T>());

			m_Ptr = static_cast<T*>(InPtr);
			m_Counter = new PtrControlBlock();
			InternalAddWeakRef();
		}

		FORCEINLINE void InternalConstructWeak(const TPtrBase& Other)
		{
			m_Ptr = Other.m_Ptr;
			m_Counter = Other.m_Counter;
			InternalAddWeakRef();
		}

		template<typename TOther>
		FORCEINLINE void InternalConstructWeak(const TPtrBase<TOther>& Other)
		{
			static_assert(std::is_convertible<TOther, T>());

			m_Ptr = static_cast<T*>(Other.m_Ptr);
			m_Counter = Other.m_Counter;
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
			m_Ptr = nullptr;
			m_Counter = nullptr;
		}

		T* m_Ptr;
		PtrControlBlock* m_Counter;
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

		FORCEINLINE explicit TSharedPtr(T* InPtr) noexcept
			: TBase()
		{
			TBase::InternalConstructStrong(InPtr);
		}

		FORCEINLINE TSharedPtr(const TSharedPtr& Other) noexcept
			: TBase()
		{
			TBase::InternalConstructStrong(Other);
		}

		FORCEINLINE TSharedPtr(TSharedPtr&& Other) noexcept
			: TBase()
		{
			TBase::InternalMove(Move(Other));
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr(const TSharedPtr<TOther>& Other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>());
			TBase::template InternalConstructStrong<TOther>(Other);
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr(TSharedPtr<TOther>&& Other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>());
			TBase::template InternalMove<TOther>(Move(Other));
		}

		template<typename TOther>
		FORCEINLINE explicit TSharedPtr(const TWeakPtr<TOther>& Other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>());
			TBase::template InternalConstructStrong<TOther>(Other);
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr(TUniquePtr<TOther>&& Other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>());
			TBase::template InternalConstructStrong<TOther>(Other.Release());
		}

		FORCEINLINE ~TSharedPtr()
		{
			Reset();
		}

		FORCEINLINE void Reset() noexcept
		{
			TBase::InternalDestructStrong();
		}

		FORCEINLINE void Swap(TSharedPtr& Other) noexcept
		{
			TBase::InternalSwap(Other);
		}

		FORCEINLINE bool IsUnique() const noexcept
		{
			return (TBase::GetStrongReferences() == 1);
		}

		FORCEINLINE TSharedPtr& operator=(const TSharedPtr& Other) noexcept
		{
			if (this != std::addressof(Other))
			{
				Reset();
				TBase::InternalConstructStrong(Other);
			}

			return *this;
		}

		FORCEINLINE TSharedPtr& operator=(TSharedPtr&& Other) noexcept
		{
			if (this != std::addressof(Other))
			{
				Reset();
				TBase::InternalMove(Move(Other));
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr& operator=(const TSharedPtr<TOther>& Other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>());

			if (this != std::addressof(Other))
			{
				Reset();
				TBase::template InternalConstructStrong<TOther>(Other);
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TSharedPtr& operator=(TSharedPtr<TOther>&& Other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>());

			if (this != std::addressof(Other))
			{
				Reset();
				TBase::template InternalMove<TOther>(Move(Other));
			}

			return *this;
		}

		FORCEINLINE TSharedPtr& operator=(T* InPtr) noexcept
		{
			if (this->m_Ptr != InPtr)
			{
				Reset();
				TBase::InternalConstructStrong(InPtr);
			}

			return *this;
		}

		FORCEINLINE TSharedPtr& operator=(std::nullptr_t) noexcept
		{
			Reset();
			return *this;
		}

		FORCEINLINE bool operator==(const TSharedPtr& Other) const noexcept
		{
			return (TBase::m_Ptr == Other.m_Ptr);
		}

		FORCEINLINE bool operator!=(const TSharedPtr& Other) const noexcept
		{
			return (TBase::m_Ptr != Other.m_Ptr);
		}

		FORCEINLINE bool operator==(TSharedPtr&& Other) const noexcept
		{
			return (TBase::m_Ptr == Other.m_Ptr);
		}

		FORCEINLINE bool operator!=(TSharedPtr&& Other) const noexcept
		{
			return (TBase::m_Ptr != Other.m_Ptr);
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

		FORCEINLINE TWeakPtr(const TSharedPtr<T>& InPtr) noexcept
			: TBase()
		{
			TBase::InternalConstructWeak(InPtr);
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr(const TSharedPtr<TOther>& InPtr) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
			TBase::template InternalConstructWeak<TOther>(InPtr);
		}

		FORCEINLINE TWeakPtr(const TWeakPtr& Other) noexcept
			: TBase()
		{
			TBase::InternalConstructWeak(Other);
		}

		FORCEINLINE TWeakPtr(TWeakPtr&& Other) noexcept
			: TBase()
		{
			TBase::InternalMove(Move(Other));
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr(const TWeakPtr<TOther>& Other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
			TBase::template InternalConstructWeak<TOther>(Other);
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr(TWeakPtr<TOther>&& Other) noexcept
			: TBase()
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");
			TBase::template InternalMove<TOther>(Move(Other));
		}

		FORCEINLINE ~TWeakPtr()
		{
			Reset();
		}

		FORCEINLINE void Reset() noexcept
		{
			TBase::InternalDestructWeak();
		}

		FORCEINLINE void Swap(TWeakPtr& Other) noexcept
		{
			TBase::InternalSwap(Other);
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

		FORCEINLINE TWeakPtr& operator=(const TWeakPtr& Other) noexcept
		{
			if (this != std::addressof(Other))
			{
				Reset();
				TBase::InternalConstructWeak(Other);
			}

			return *this;
		}

		FORCEINLINE TWeakPtr& operator=(TWeakPtr&& Other) noexcept
		{
			if (this != std::addressof(Other))
			{
				Reset();
				TBase::InternalMove(Move(Other));
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr& operator=(const TWeakPtr<TOther>& Other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");

			if (this != std::addressof(Other))
			{
				Reset();
				TBase::template InternalConstructWeak<TOther>(Other);
			}

			return *this;
		}

		template<typename TOther>
		FORCEINLINE TWeakPtr& operator=(TWeakPtr<TOther>&& Other) noexcept
		{
			static_assert(std::is_convertible<TOther, T>(), "TWeakPtr: Trying to convert non-convertable types");

			if (this != std::addressof(Other))
			{
				Reset();
				TBase::InternalMove(Move(Other));
			}

			return *this;
		}

		FORCEINLINE TWeakPtr& operator=(T* InPtr) noexcept
		{
			if (TBase::m_Ptr != InPtr)
			{
				Reset();
				TBase::InternalConstructWeak(InPtr);
			}

			return *this;
		}

		FORCEINLINE TWeakPtr& operator=(std::nullptr_t) noexcept
		{
			Reset();
			return *this;
		}

		FORCEINLINE bool operator==(const TWeakPtr& Other) const noexcept
		{
			return (TBase::m_Ptr == Other.m_Ptr);
		}

		FORCEINLINE bool operator!=(const TWeakPtr& Other) const noexcept
		{
			return (TBase::m_Ptr != Other.m_Ptr);
		}

		FORCEINLINE bool operator==(TWeakPtr&& Other) const noexcept
		{
			return (TBase::m_Ptr == Other.m_Ptr);
		}

		FORCEINLINE bool operator!=(TWeakPtr&& Other) const noexcept
		{
			return (TBase::m_Ptr != Other.m_Ptr);
		}
	};

	/*
	* Creates a new object together with a SharedPtr
	*/
	template<typename T, typename... TArgs>
	TSharedPtr<T> MakeShared(TArgs&&... Args) noexcept
	{
		T* RefCountedPtr = new T(Forward<TArgs>(Args)...);
		return Move(TSharedPtr<T>(RefCountedPtr));
	}
}