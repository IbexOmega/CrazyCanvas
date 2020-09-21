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

		inline PtrControlBlock()
			: m_WeakReferences(0)
			, m_StrongReferences(0)
		{
		}

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
	* Deleter functors
	*/
	template<typename T>
	struct TDelete
	{
		void operator()(T* pPtr)
		{
			delete pPtr;
		}
	};

	template<typename T>
	struct TDelete<T[]>
	{
		void operator()(T* pPtr)
		{
			delete[] pPtr;
		}
	};

	/*
	* Base class for TWeak- and TSharedPtr
	*/
	template<typename T, typename D>
	class TPtrBase
	{
	public:
		template<typename TOther, typename DOther>
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
			, m_Deleter()
		{
			static_assert(std::is_array_v<T> == std::is_array_v<D>, "Scalar types must have scalar TDelete");
			static_assert(std::is_invocable<D, T*>(), "TDelete must be a callable");
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
					if (m_pCounter->GetWeakReferences() <= 0)
					{
						delete m_pCounter;
					}

					m_Deleter(m_pPtr);
				}
			}

			InternalClear();
		}

		FORCEINLINE void InternalReleaseWeakRef() noexcept
		{
			// If the object has a Ptr there must be a Counter or something went wrong
			if (m_pPtr)
			{
				VALIDATE(m_pCounter != nullptr);

				m_pCounter->ReleaseWeakRef();
				if (m_pCounter->GetWeakReferences() <= 0)
				{
					if (m_pCounter->GetStrongReferences() <= 0)
					{
						delete m_pCounter;
					}
				}
			}

			InternalClear();
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

		template<typename TOther, typename DOther>
		FORCEINLINE void InternalMove(TPtrBase<TOther, DOther>&& other) noexcept
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
			m_pCounter = DBG_NEW PtrControlBlock();
			InternalAddStrongRef();
		}

		template<typename TOther>
		FORCEINLINE void InternalConstructStrong(TOther* pPtr)
		{
			static_assert(std::is_convertible<TOther*, T*>());

			m_pPtr = static_cast<T*>(pPtr);
			m_pCounter = DBG_NEW PtrControlBlock();
			InternalAddStrongRef();
		}

		FORCEINLINE void InternalConstructStrong(const TPtrBase& other)
		{
			m_pPtr = other.m_pPtr;
			m_pCounter = other.m_pCounter;
			InternalAddStrongRef();
		}

		template<typename TOther, typename DOther>
		FORCEINLINE void InternalConstructStrong(const TPtrBase<TOther, DOther>& other)
		{
			static_assert(std::is_convertible<TOther*, T*>());

			m_pPtr = static_cast<T*>(other.m_pPtr);
			m_pCounter = other.m_pCounter;
			InternalAddStrongRef();
		}

		FORCEINLINE void InternalConstructWeak(T* pPtr)
		{
			m_pPtr = pPtr;
			m_pCounter = DBG_NEW PtrControlBlock();
			InternalAddWeakRef();
		}

		template<typename TOther>
		FORCEINLINE void InternalConstructWeak(TOther* pPtr)
		{
			static_assert(std::is_convertible<TOther*, T*>());

			m_pPtr = static_cast<T*>(pPtr);
			m_pCounter = DBG_NEW PtrControlBlock();
			InternalAddWeakRef();
		}

		FORCEINLINE void InternalConstructWeak(const TPtrBase& other)
		{
			m_pPtr = other.m_pPtr;
			m_pCounter = other.m_pCounter;
			InternalAddWeakRef();
		}

		template<typename TOther, typename DOther>
		FORCEINLINE void InternalConstructWeak(const TPtrBase<TOther, DOther>& other)
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

	protected:
		T* m_pPtr;
		PtrControlBlock* m_pCounter;
		D m_Deleter;
	};

	/*
	* Forward Declarations
	*/
	template<typename TOther>
	class TWeakPtr;

	/*
	* TSharedPtr - RefCounted Pointer for scalar pointers
	*/
	template<typename T>
	class TSharedPtr : public TPtrBase<T, TDelete<T>>
	{
		using TBase = TPtrBase<T, TDelete<T>>;

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

		FORCEINLINE T* operator->() const noexcept
		{
			return Get();
		}

		FORCEINLINE T& operator*() const noexcept
		{
			return (*m_pPtr);
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
	* TSharedPtr - RefCounted Pointer for array pointers
	*/
	template<typename T>
	class TSharedPtr<T[]> : public TPtrBase<T, TDelete<T[]>>
	{
		using TBase = TPtrBase<T, TDelete<T[]>>;

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

		FORCEINLINE T& operator[](uint32 Index) const noexcept
		{
			VALIDATE(m_pPtr != nullptr);
			return m_pPtr[Index];
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
	* TWeakPtr - Weak Pointer similar for scalar types
	*/
	template<typename T>
	class TWeakPtr : public TPtrBase<T, TDelete<T>>
	{
		using TBase = TPtrBase<T, TDelete<T>>;

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

		FORCEINLINE T* operator->() const noexcept
		{
			return Get();
		}

		FORCEINLINE T& operator*() const noexcept
		{
			return (*m_pPtr);
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
	* TWeakPtr - Weak Pointer for array types
	*/
	template<typename T>
	class TWeakPtr<T[]> : public TPtrBase<T, TDelete<T[]>>
	{
		using TBase = TPtrBase<T, TDelete<T[]>>;

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

		FORCEINLINE TSharedPtr<T[]> MakeShared() noexcept
		{
			const TWeakPtr& This = *this;
			return Move(TSharedPtr<T[]>(This));
		}

		FORCEINLINE T& operator[](uint32 Index) const noexcept
		{
			VALIDATE(m_pPtr != nullptr);
			return m_pPtr[Index];
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
	std::enable_if_t<!std::is_array_v<T>, TSharedPtr<T>> MakeShared(TArgs&&... args) noexcept
	{
		T* pRefCountedPtr = DBG_NEW T(Forward<TArgs>(args)...);
		return Move(TSharedPtr<T>(pRefCountedPtr));
	}

	/*
	* Creates a new object together with a SharedPtr
	*/
	template<typename T>
	std::enable_if_t<std::is_array_v<T>, TSharedPtr<T>> MakeShared(uint32 size) noexcept
	{
		using TType = TRemoveExtent<T>;

		TType* pRefCountedPtr = DBG_NEW TType[size];
		return Move(TSharedPtr<T>(pRefCountedPtr));
	}
}