#pragma once
#include "TUtilities.h"

// Disable the DLL- linkage warning for now
#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(disable : 4251)
#endif

namespace LambdaEngine
{
	/*
	* Dynamic Array similar to std::vector
	*/
	template<typename T>
	class TArray
	{
	public:
		typedef uint32 SizeType;

		/*
		* Constant Iterator
		*/
		class ConstIterator
		{
			friend class TArray;

		public:
			~ConstIterator() = default;

			FORCEINLINE ConstIterator(T* InPtr = nullptr)
				: m_Ptr(InPtr)
			{
			}

			FORCEINLINE const T* operator->() const
			{
				return m_Ptr;
			}

			FORCEINLINE const T& operator*() const
			{
				return *m_Ptr;
			}

			FORCEINLINE ConstIterator operator++()
			{
				m_Ptr++;
				return *this;
			}

			FORCEINLINE ConstIterator operator++(int32)
			{
				ConstIterator Temp = *this;
				m_Ptr++;
				return Temp;
			}

			FORCEINLINE ConstIterator operator--()
			{
				m_Ptr--;
				return *this;
			}

			FORCEINLINE ConstIterator operator--(int32)
			{
				ConstIterator Temp = *this;
				m_Ptr--;
				return Temp;
			}

			FORCEINLINE ConstIterator operator+(int32 Offset) const
			{
				ConstIterator Temp = *this;
				return Temp += Offset;
			}

			FORCEINLINE ConstIterator operator-(int32 Offset) const
			{
				ConstIterator Temp = *this;
				return Temp -= Offset;
			}

			FORCEINLINE ConstIterator& operator+=(int32 Offset)
			{
				m_Ptr += Offset;
				return *this;
			}

			FORCEINLINE ConstIterator& operator-=(int32 Offset)
			{
				m_Ptr -= Offset;
				return *this;
			}

			FORCEINLINE bool operator==(const ConstIterator& Other) const
			{
				return (m_Ptr == Other.m_Ptr);
			}

			FORCEINLINE bool operator!=(const ConstIterator& Other) const
			{
				return (m_Ptr != Other.m_Ptr);
			}

			FORCEINLINE bool operator<(const ConstIterator& Other) const
			{
				return m_Ptr < Other.m_Ptr;
			}

			FORCEINLINE bool operator<=(const ConstIterator& Other) const
			{
				return m_Ptr <= Other.m_Ptr;
			}

			FORCEINLINE bool operator>(const ConstIterator& Other) const
			{
				return m_Ptr > Other.m_Ptr;
			}

			FORCEINLINE bool operator>=(const ConstIterator& Other) const
			{
				return m_Ptr >= Other.m_Ptr;
			}

		protected:
			T* m_Ptr;
		};

		/*
		* Standard Iterator
		*/
		class Iterator : public ConstIterator
		{
			friend class TArray;

		public:
			~Iterator() = default;

			FORCEINLINE Iterator(T* InPtr = nullptr)
				: ConstIterator(InPtr)
			{
			}

			FORCEINLINE T* operator->() const
			{
				return const_cast<T*>(ConstIterator::operator->());
			}

			FORCEINLINE T& operator*() const
			{
				return const_cast<T&>(ConstIterator::operator*());
			}

			FORCEINLINE Iterator operator++()
			{
				ConstIterator::operator++();
				return *this;
			}

			FORCEINLINE Iterator operator++(int32)
			{
				Iterator Temp = *this;
				ConstIterator::operator++();
				return Temp;
			}

			FORCEINLINE Iterator operator--()
			{
				ConstIterator::operator--();
				return *this;
			}

			FORCEINLINE Iterator operator--(int32)
			{
				Iterator Temp = *this;
				ConstIterator::operator--();
				return Temp;
			}

			FORCEINLINE Iterator operator+(int32 Offset) const
			{
				Iterator Temp = *this;
				return Temp += Offset;
			}

			FORCEINLINE Iterator operator-(int32 Offset) const
			{
				Iterator Temp = *this;
				return Temp -= Offset;
			}

			FORCEINLINE Iterator& operator+=(int32 Offset)
			{
				ConstIterator::operator+=(Offset);
				return *this;
			}

			FORCEINLINE Iterator& operator-=(int32 Offset)
			{
				ConstIterator::operator-=(Offset);
				return *this;
			}
		};

		/*
		* Reverse Constant Iterator
		* Stores for example End(), but will reference End() - 1
		*/
		class ReverseConstIterator
		{
			friend class TArray;

		public:
			~ReverseConstIterator() = default;

			FORCEINLINE explicit ReverseConstIterator(T* InPtr = nullptr)
				: m_Ptr(InPtr)
			{
			}

			FORCEINLINE const T* operator->() const
			{
				return (m_Ptr - 1);
			}

			FORCEINLINE const T& operator*() const
			{
				return *(m_Ptr - 1);
			}

			FORCEINLINE ReverseConstIterator operator++()
			{
				m_Ptr--;
				return *this;
			}

			FORCEINLINE ReverseConstIterator operator++(int32)
			{
				ConstIterator Temp = *this;
				m_Ptr--;
				return Temp;
			}

			FORCEINLINE ReverseConstIterator operator--()
			{
				m_Ptr++;
				return *this;
			}

			FORCEINLINE ReverseConstIterator operator--(int32)
			{
				ReverseConstIterator Temp = *this;
				m_Ptr++;
				return Temp;
			}

			FORCEINLINE ReverseConstIterator operator+(int32 Offset) const
			{
				ReverseConstIterator Temp = *this;
				return Temp += Offset;
			}

			FORCEINLINE ReverseConstIterator operator-(int32 Offset) const
			{
				ReverseConstIterator Temp = *this;
				return Temp -= Offset;
			}

			FORCEINLINE ReverseConstIterator& operator+=(int32 Offset)
			{
				m_Ptr -= Offset;
				return *this;
			}

			FORCEINLINE ReverseConstIterator& operator-=(int32 Offset)
			{
				m_Ptr += Offset;
				return *this;
			}

			FORCEINLINE bool operator==(const ReverseConstIterator& Other) const
			{
				return (m_Ptr == Other.m_Ptr);
			}

			FORCEINLINE bool operator!=(const ReverseConstIterator& Other) const
			{
				return (m_Ptr != Other.m_Ptr);
			}

		protected:
			T* m_Ptr;
		};

		/*
		* Standard Reverse Iterator
		*/
		class ReverseIterator : public ReverseConstIterator
		{
			friend class TArray;

		public:
			~ReverseIterator() = default;

			FORCEINLINE ReverseIterator(T* InPtr = nullptr)
				: ReverseConstIterator(InPtr)
			{
			}

			FORCEINLINE T* operator->() const
			{
				return const_cast<T*>(ReverseConstIterator::operator->());
			}

			FORCEINLINE T& operator*() const
			{
				return const_cast<T&>(ReverseConstIterator::operator*());
			}

			FORCEINLINE ReverseIterator operator++()
			{
				ReverseConstIterator::operator++();
				return *this;
			}

			FORCEINLINE ReverseIterator operator++(int32)
			{
				ReverseIterator Temp = *this;
				ReverseConstIterator::operator++();
				return Temp;
			}

			FORCEINLINE ReverseIterator operator--()
			{
				ReverseConstIterator::operator--();
				return *this;
			}

			FORCEINLINE ReverseIterator operator--(int32)
			{
				ReverseIterator Temp = *this;
				ReverseConstIterator::operator--();
				return Temp;
			}

			FORCEINLINE ReverseIterator operator+(int32 Offset) const
			{
				ReverseIterator Temp = *this;
				return Temp += Offset;
			}

			FORCEINLINE ReverseIterator operator-(int32 Offset) const
			{
				ReverseIterator Temp = *this;
				return Temp -= Offset;
			}

			FORCEINLINE ReverseIterator& operator+=(int32 Offset)
			{
				ReverseConstIterator::operator+=(Offset);
				return *this;
			}

			FORCEINLINE ReverseIterator& operator-=(int32 Offset)
			{
				ReverseConstIterator::operator-=(Offset);
				return *this;
			}
		};

		/*
		* TArray API
		*/
	public:
		FORCEINLINE TArray() noexcept
			: m_Data(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
		}

		FORCEINLINE explicit TArray(SizeType InSize) noexcept
			: m_Data(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			InternalConstruct(InSize);
		}

		FORCEINLINE explicit TArray(SizeType InSize, const T& Value) noexcept
			: m_Data(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			InternalConstruct(InSize, Value);
		}

		FORCEINLINE explicit TArray(ConstIterator InBegin, ConstIterator InEnd) noexcept
			: m_Data(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			InternalConstruct(InBegin, InEnd);
		}

		FORCEINLINE TArray(std::initializer_list<T> IList) noexcept
			: m_Data(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			// TODO: Get rid of const_cast
			InternalConstruct(const_cast<T*>(IList.begin()), const_cast<T*>(IList.end()));
		}

		FORCEINLINE TArray(const TArray& Other) noexcept
			: m_Data(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			InternalConstruct(Other.Begin(), Other.End());
		}

		FORCEINLINE TArray(TArray&& Other) noexcept
			: m_Data(nullptr)
			, m_Size(0)
			, m_Capacity(0)
		{
			InternalMove(Move(Other));
		}

		FORCEINLINE ~TArray()
		{
			Clear();

			InternalReleaseData();

			m_Capacity = 0;
			m_Data = nullptr;
		}

		FORCEINLINE void Clear() noexcept
		{
			InternalDestructRange(m_Data, m_Data + m_Size);
			m_Size = 0;
		}

		FORCEINLINE void Assign(SizeType InSize) noexcept
		{
			Clear();
			InternalConstruct(InSize);
		}

		FORCEINLINE void Assign(SizeType InSize, const T& Value) noexcept
		{
			Clear();
			InternalConstruct(InSize, Value);
		}

		FORCEINLINE void Assign(ConstIterator InBegin, ConstIterator InEnd) noexcept
		{
			Clear();
			InternalConstruct(InBegin, InEnd);
		}

		FORCEINLINE void Assign(std::initializer_list<T> IList) noexcept
		{
			Clear();

			// TODO: Get rid of const_cast
			InternalConstruct(const_cast<T*>(IList.begin()), const_cast<T*>(IList.end()));
		}

		FORCEINLINE void Resize(SizeType InSize) noexcept
		{
			if (InSize > m_Size)
			{
				if (InSize >= m_Capacity)
				{
					const SizeType NewCapacity = InternalGetResizeFactor(InSize);
					InternalRealloc(NewCapacity);
				}

				InternalDefaultConstructRange(m_Data + m_Size, m_Data + InSize);
			}
			else if (InSize < m_Size)
			{
				InternalDestructRange(m_Data + InSize, m_Data + m_Size);
			}

			m_Size = InSize;
		}

		FORCEINLINE void Resize(SizeType InSize, const T& Value) noexcept
		{
			if (InSize > m_Size)
			{
				if (InSize >= m_Capacity)
				{
					const SizeType NewCapacity = InternalGetResizeFactor(InSize);
					InternalRealloc(NewCapacity);
				}

				InternalCopyEmplace(InSize - m_Size, Value, m_Data + m_Size);
			}
			else if (InSize < m_Size)
			{
				InternalDestructRange(m_Data + InSize, m_Data + m_Size);
			}

			m_Size = InSize;
		}

		FORCEINLINE void Reserve(SizeType InCapacity) noexcept
		{
			if (InCapacity != m_Capacity)
			{
				SizeType OldSize = m_Size;
				if (InCapacity < m_Size)
				{
					m_Size = InCapacity;
				}

				T* TempData = InternalAllocateElements(InCapacity);
				InternalMoveEmplace(m_Data, m_Data + m_Size, TempData);
				InternalDestructRange(m_Data, m_Data + OldSize);
				InternalReleaseData();

				m_Data = TempData;
				m_Capacity = InCapacity;
			}
		}

		template<typename... TArgs>
		FORCEINLINE T& EmplaceBack(TArgs&&... Args) noexcept
		{
			if (m_Size >= m_Capacity)
			{
				const SizeType NewCapacity = InternalGetResizeFactor();
				InternalRealloc(NewCapacity);
			}

			T* DataEnd = m_Data + m_Size;
			new(reinterpret_cast<void*>(DataEnd)) T(Forward<TArgs>(Args)...);
			m_Size++;
			return (*DataEnd);
		}

		FORCEINLINE T& PushBack(const T& Element) noexcept
		{
			return EmplaceBack(Element);
		}

		FORCEINLINE T& PushBack(T&& Element) noexcept
		{
			return EmplaceBack(Move(Element));
		}

		template<typename... TArgs>
		FORCEINLINE Iterator Emplace(ConstIterator Pos, TArgs&&... Args) noexcept
		{
			// Emplace back
			if (Pos == End())
			{
				const SizeType OldSize = m_Size;
				EmplaceBack(Forward<TArgs>(Args)...);
				return Iterator(m_Data + OldSize);
			}

			// Emplace
			const SizeType Index = InternalIndex(Pos);
			T* DataBegin = m_Data + Index;
			if (m_Size >= m_Capacity)
			{
				const SizeType NewCapacity = InternalGetResizeFactor();
				InternalEmplaceRealloc(NewCapacity, DataBegin, 1);
				DataBegin = m_Data + Index;
			}
			else
			{
				// Construct the range so that we can move to it
				T* DataEnd = m_Data + m_Size;
				InternalDefaultConstructRange(DataEnd, DataEnd + 1);
				InternalMemmoveForward(DataBegin, DataEnd, DataEnd);
				InternalDestruct(DataBegin);
			}

			new (reinterpret_cast<void*>(DataBegin)) T(Forward<TArgs>(Args)...);
			m_Size++;
			return Iterator(DataBegin);
		}

		FORCEINLINE Iterator Insert(ConstIterator Pos, const T& Value) noexcept
		{
			return Emplace(Pos, Value);
		}

		FORCEINLINE Iterator Insert(ConstIterator Pos, T&& Value) noexcept
		{
			return Emplace(Pos, Move(Value));
		}

		FORCEINLINE Iterator Insert(ConstIterator Pos, std::initializer_list<T> IList) noexcept
		{
			// Insert at end
			if (Pos == End())
			{
				const SizeType OldSize = m_Size;
				for (const T& Value : IList)
				{
					EmplaceBack(Move(Value));
				}

				return Iterator(m_Data + OldSize);
			}

			// Insert
			const SizeType ListSize	= static_cast<SizeType>(IList.size());
			const SizeType NewSize	= m_Size + ListSize;
			const SizeType Index	= InternalIndex(Pos);

			T* RangeBegin = m_Data + Index;
			if (NewSize >= m_Capacity)
			{
				const SizeType NewCapacity = InternalGetResizeFactor(NewSize);
				InternalEmplaceRealloc(NewCapacity, RangeBegin, ListSize);
				RangeBegin = m_Data + Index;
			}
			else
			{
				// Construct the range so that we can move to it
				T* DataEnd		= m_Data + m_Size;
				T* NewDataEnd	= m_Data + m_Size + ListSize;
				T* RangeEnd		= RangeBegin + ListSize;
				InternalDefaultConstructRange(DataEnd, NewDataEnd);
				InternalMemmoveForward(RangeBegin, DataEnd, NewDataEnd - 1);
				InternalDestructRange(RangeBegin, RangeEnd);
			}

			// TODO: Get rid of const_cast
			InternalMoveEmplace(const_cast<T*>(IList.begin()), const_cast<T*>(IList.end()), RangeBegin);
			m_Size = NewSize;
			return Iterator(RangeBegin);
		}

		FORCEINLINE Iterator Insert(ConstIterator Pos, ConstIterator InBegin, ConstIterator InEnd) noexcept
		{
			// Insert at end
			if (Pos == End())
			{
				const SizeType OldSize = m_Size;
				for (ConstIterator It = InBegin; It != InEnd; It++)
				{
					EmplaceBack(*It);
				}

				return Iterator(m_Data + OldSize);
			}

			// Insert
			const SizeType RangeSize	= InternalDistance(InBegin.m_Ptr, InEnd.m_Ptr);
			const SizeType NewSize		= m_Size + RangeSize;
			const SizeType Index		= InternalIndex(Pos);

			T* RangeBegin = m_Data + Index;
			if (NewSize >= m_Capacity)
			{
				const SizeType NewCapacity = InternalGetResizeFactor(NewSize);
				InternalEmplaceRealloc(NewCapacity, RangeBegin, RangeSize);
				RangeBegin = m_Data + Index;
			}
			else
			{
				// Construct the range so that we can move to it
				T* DataEnd		= m_Data + m_Size;
				T* NewDataEnd	= m_Data + m_Size + RangeSize;
				T* RangeEnd		= RangeBegin + RangeSize;
				InternalDefaultConstructRange(DataEnd, NewDataEnd);
				InternalMemmoveForward(RangeBegin, DataEnd, NewDataEnd - 1);
				InternalDestructRange(RangeBegin, RangeEnd);
			}

			InternalCopyEmplace(InBegin.m_Ptr, InEnd.m_Ptr, RangeBegin);
			m_Size = NewSize;
			return Iterator(RangeBegin);
		}

		FORCEINLINE void PopBack() noexcept
		{
			if (m_Size > 0)
			{
				InternalDestruct(m_Data + (--m_Size));
			}
		}

		FORCEINLINE Iterator Erase(ConstIterator Pos) noexcept
		{
			VALIDATE(InternalIsIteratorOwner(Pos));

			// Erase at end
			if (Pos == End())
			{
				PopBack();
				return End();
			}

			// Erase
			T* DataBegin = Pos.m_Ptr;
			T* DataEnd = m_Data + m_Size;
			InternalMemmoveBackwards(DataBegin + 1, DataEnd, DataBegin);
			InternalDestruct(DataEnd - 1);

			m_Size--;
			return Iterator(DataBegin);
		}

		FORCEINLINE Iterator Erase(ConstIterator InBegin, ConstIterator InEnd) noexcept
		{
			VALIDATE(InBegin < InEnd);
			VALIDATE(InternalIsRangeOwner(InBegin, InEnd));

			T* DataBegin = InBegin.m_Ptr;
			T* DataEnd = InEnd.m_Ptr;
			const SizeType ElementCount = InternalDistance(DataBegin, DataEnd);
			if (InEnd >= End())
			{
				InternalDestructRange(DataBegin, DataEnd);
			}
			else
			{
				T* RealEnd = m_Data + m_Size;
				InternalMemmoveBackwards(DataEnd, RealEnd, DataBegin);
				InternalDestructRange(RealEnd - ElementCount, RealEnd);
			}

			m_Size -= ElementCount;
			return Iterator(DataBegin);
		}

		FORCEINLINE void Swap(TArray& Other) noexcept
		{
			if (this != std::addressof(Other))
			{
				T* TempPtr = m_Data;
				SizeType TempSize = m_Size;
				SizeType TempCapacity = m_Capacity;

				m_Data = Other.m_Data;
				m_Size = Other.m_Size;
				m_Capacity = Other.m_Capacity;

				Other.m_Data = TempPtr;
				Other.m_Size = TempSize;
				Other.m_Capacity = TempCapacity;
			}
		}

		FORCEINLINE void ShrinkToFit() noexcept
		{
			if (m_Capacity > m_Size)
			{
				InternalRealloc(m_Size);
			}
		}

		FORCEINLINE bool IsEmpty() const noexcept
		{
			return (m_Size == 0);
		}

		FORCEINLINE Iterator Begin() noexcept
		{
			return Iterator(m_Data);
		}

		FORCEINLINE Iterator End() noexcept
		{
			return Iterator(m_Data + m_Size);
		}

		FORCEINLINE ConstIterator Begin() const noexcept
		{
			return ConstIterator(m_Data);
		}

		FORCEINLINE ConstIterator End() const noexcept
		{
			return ConstIterator(m_Data + m_Size);
		}

		FORCEINLINE ConstIterator ConstBegin() const noexcept
		{
			return ConstIterator(m_Data);
		}

		FORCEINLINE ConstIterator ConstEnd() const noexcept
		{
			return ConstIterator(m_Data + m_Size);
		}

		FORCEINLINE ReverseIterator ReverseBegin() noexcept
		{
			return ReverseIterator(m_Data + m_Size);
		}

		FORCEINLINE ReverseIterator ReverseEnd() noexcept
		{
			return ReverseIterator(m_Data);
		}

		FORCEINLINE ReverseConstIterator ReverseBegin() const noexcept
		{
			return ReverseConstIterator(m_Data + m_Size);
		}

		FORCEINLINE ReverseConstIterator ReverseEnd() const noexcept
		{
			return ReverseConstIterator(m_Data);
		}

		FORCEINLINE ReverseConstIterator ConstReverseBegin() const noexcept
		{
			return ReverseConstIterator(m_Data + m_Size);
		}

		FORCEINLINE ReverseConstIterator ConstReverseEnd() const noexcept
		{
			return ReverseConstIterator(m_Data);
		}

		FORCEINLINE T& GetFront() noexcept
		{
			VALIDATE(m_Size > 0);
			return m_Data[0];
		}

		FORCEINLINE const T& GetFront() const noexcept
		{
			VALIDATE(m_Size > 0);
			return m_Data[0];
		}

		FORCEINLINE T& GetBack() noexcept
		{
			VALIDATE(m_Size > 0);
			return m_Data[m_Size - 1];
		}

		FORCEINLINE const T& GetBack() const noexcept
		{
			VALIDATE(m_Size > 0);
			return m_Data[m_Size - 1];
		}

		FORCEINLINE T* GetData() noexcept
		{
			return m_Data;
		}

		FORCEINLINE const T* GetData() const noexcept
		{
			return m_Data;
		}

		FORCEINLINE SizeType GetSize() const noexcept
		{
			return m_Size;
		}

		FORCEINLINE SizeType GetCapacity() const noexcept
		{
			return m_Capacity;
		}

		FORCEINLINE T& GetElementAt(SizeType Index) noexcept
		{
			VALIDATE(Index < m_Size);
			return m_Data[Index];
		}

		FORCEINLINE const T& GetElementAt(SizeType Index) const noexcept
		{
			VALIDATE(Index < m_Size);
			return m_Data[Index];
		}

		FORCEINLINE TArray& operator=(const TArray& Other) noexcept
		{
			if (this != std::addressof(Other))
			{
				Clear();
				InternalConstruct(Other.Begin(), Other.End());
			}

			return *this;
		}

		FORCEINLINE TArray& operator=(TArray&& Other) noexcept
		{
			if (this != std::addressof(Other))
			{
				Clear();
				InternalReleaseData();
				InternalMove(Move(Other));
			}

			return *this;
		}

		FORCEINLINE TArray& operator=(std::initializer_list<T> IList) noexcept
		{
			Assign(IList);
			return *this;
		}

		FORCEINLINE T& operator[](SizeType Index) noexcept
		{
			return GetElementAt(Index);
		}

		FORCEINLINE const T& operator[](SizeType Index) const noexcept
		{
			return GetElementAt(Index);
		}

		/*
		* STL iterator functions, Only here so that you can use Range for-loops
		*/
	public:
		FORCEINLINE Iterator begin() noexcept
		{
			return Iterator(m_Data);
		}

		FORCEINLINE Iterator end() noexcept
		{
			return Iterator(m_Data + m_Size);
		}

		FORCEINLINE ConstIterator begin() const noexcept
		{
			return ConstIterator(m_Data);
		}

		FORCEINLINE ConstIterator end() const noexcept
		{
			return ConstIterator(m_Data + m_Size);
		}

		FORCEINLINE ConstIterator cbegin() const noexcept
		{
			return ConstIterator(m_Data);
		}

		FORCEINLINE ConstIterator cend() const noexcept
		{
			return ConstIterator(m_Data + m_Size);
		}

		FORCEINLINE ReverseIterator rbegin() noexcept
		{
			return ReverseIterator(m_Data);
		}

		FORCEINLINE ReverseIterator rend() noexcept
		{
			return ReverseIterator(m_Data + m_Size);
		}

		FORCEINLINE ReverseConstIterator rbegin() const noexcept
		{
			return ReverseConstIterator(m_Data);
		}

		FORCEINLINE ReverseConstIterator rend() const noexcept
		{
			return ReverseConstIterator(m_Data + m_Size);
		}

		FORCEINLINE ReverseConstIterator crbegin() const noexcept
		{
			return ReverseConstIterator(m_Data);
		}

		FORCEINLINE ReverseConstIterator crend() const noexcept
		{
			return ReverseConstIterator(m_Data + m_Size);
		}

	private:
		// Check is the iterator belongs to this TArray
		FORCEINLINE bool InternalIsRangeOwner(ConstIterator InBegin, ConstIterator InEnd)
		{
			return (InBegin < InEnd) && (InBegin >= Begin()) && (InEnd <= End());
		}

		FORCEINLINE bool InternalIsIteratorOwner(ConstIterator It)
		{
			return (It >= Begin()) && (It <= End());
		}

		// Helpers
		FORCEINLINE SizeType InternalDistance(ConstIterator InBegin, ConstIterator InEnd)
		{
			return InternalDistance(InBegin.m_Ptr, InEnd.m_Ptr);
		}

		FORCEINLINE SizeType InternalDistance(const T* InBegin, const T* InEnd)
		{
			return static_cast<SizeType>(InEnd - InBegin);
		}

		FORCEINLINE SizeType InternalIndex(ConstIterator Pos)
		{
			return InternalIndex(Pos.m_Ptr);
		}

		FORCEINLINE SizeType InternalIndex(const T* Pos)
		{
			return static_cast<SizeType>(Pos - m_Data);
		}

		FORCEINLINE SizeType InternalGetResizeFactor() const
		{
			return InternalGetResizeFactor(m_Size);
		}

		FORCEINLINE SizeType InternalGetResizeFactor(SizeType BaseSize) const
		{
			return BaseSize + (m_Capacity)+1;
		}

		FORCEINLINE T* InternalAllocateElements(SizeType InCapacity)
		{
			constexpr SizeType ElementByteSize = sizeof(T);
			return reinterpret_cast<T*>(malloc(static_cast<size_t>(ElementByteSize) * InCapacity));
		}

		FORCEINLINE void InternalReleaseData()
		{
			if (m_Data)
			{
				free(m_Data);
			}
		}

		FORCEINLINE void InternalAllocData(SizeType InCapacity)
		{
			if (InCapacity > m_Capacity)
			{
				InternalReleaseData();

				m_Data = InternalAllocateElements(InCapacity);
				m_Capacity = InCapacity;
			}
		}

		FORCEINLINE void InternalRealloc(SizeType InCapacity)
		{
			T* TempData = InternalAllocateElements(InCapacity);
			InternalMoveEmplace(m_Data, m_Data + m_Size, TempData);
			InternalDestructRange(m_Data, m_Data + m_Size);
			InternalReleaseData();

			m_Data = TempData;
			m_Capacity = InCapacity;
		}

		FORCEINLINE void InternalEmplaceRealloc(SizeType InCapacity, T* EmplacePos, SizeType Count)
		{
			VALIDATE(InCapacity >= m_Size + Count);

			const SizeType Index = InternalIndex(EmplacePos);
			T* TempData = InternalAllocateElements(InCapacity);
			InternalMoveEmplace(m_Data, EmplacePos, TempData);
			if (EmplacePos != m_Data + m_Size)
			{
				InternalMoveEmplace(EmplacePos, m_Data + m_Size, TempData + Index + Count);
			}

			InternalDestructRange(m_Data, m_Data + m_Size);
			InternalReleaseData();

			m_Data = TempData;
			m_Capacity = InCapacity;
		}

		// Construct
		FORCEINLINE void InternalConstruct(SizeType InSize)
		{
			if (InSize > 0)
			{
				InternalAllocData(InSize);
				m_Size = InSize;
				InternalDefaultConstructRange(m_Data, m_Data + m_Size);
			}
		}

		FORCEINLINE void InternalConstruct(SizeType InSize, const T& Value)
		{
			if (InSize > 0)
			{
				InternalAllocData(InSize);
				InternalCopyEmplace(InSize, Value, m_Data);
				m_Size = InSize;
			}
		}

		FORCEINLINE void InternalConstruct(ConstIterator InBegin, ConstIterator InEnd)
		{
			const SizeType Distance = InternalDistance(InBegin, InEnd);
			if (Distance > 0)
			{
				InternalAllocData(Distance);
				InternalCopyEmplace(InBegin.m_Ptr, InEnd.m_Ptr, m_Data);
				m_Size = Distance;
			}
		}

		FORCEINLINE void InternalConstruct(T* InBegin, T* InEnd)
		{
			const SizeType Distance = InternalDistance(InBegin, InEnd);
			if (Distance > 0)
			{
				InternalAllocData(Distance);
				InternalMoveEmplace(InBegin, InEnd, m_Data);
				m_Size = Distance;
			}
		}

		FORCEINLINE void InternalMove(TArray&& Other)
		{
			m_Data = Other.m_Data;
			m_Size = Other.m_Size;
			m_Capacity = Other.m_Capacity;

			Other.m_Data = nullptr;
			Other.m_Size = 0;
			Other.m_Capacity = 0;
		}

		// Emplace
		FORCEINLINE void InternalCopyEmplace(const T* InBegin, const T* InEnd, T* Dest)
		{
			// This function assumes that there is no overlap
			if constexpr (std::is_trivially_copy_constructible<T>())
			{
				const SizeType Count = InternalDistance(InBegin, InEnd);
				const SizeType CpySize = Count * sizeof(T);
				memcpy(Dest, InBegin, CpySize);
			}
			else
			{
				while (InBegin != InEnd)
				{
					new(reinterpret_cast<void*>(Dest)) T(*InBegin);
					InBegin++;
					Dest++;
				}
			}
		}

		FORCEINLINE void InternalCopyEmplace(SizeType InSize, const T& Value, T* Dest)
		{
			T* ItEnd = Dest + InSize;
			while (Dest != ItEnd)
			{
				new(reinterpret_cast<void*>(Dest)) T(Value);
				Dest++;
			}
		}

		FORCEINLINE void InternalMoveEmplace(T* InBegin, T* InEnd, T* Dest)
		{
			// This function assumes that there is no overlap
			if constexpr (std::is_trivially_move_constructible<T>())
			{
				const SizeType Count = InternalDistance(InBegin, InEnd);
				const SizeType CpySize = Count * sizeof(T);
				memcpy(Dest, InBegin, CpySize);
			}
			else
			{
				while (InBegin != InEnd)
				{
					new(reinterpret_cast<void*>(Dest)) T(Move(*InBegin));
					InBegin++;
					Dest++;
				}
			}
		}

		FORCEINLINE void InternalMemmoveBackwards(T* InBegin, T* InEnd, T* Dest)
		{
			VALIDATE(InBegin <= InEnd);
			if (InBegin == InEnd)
			{
				return;
			}

			VALIDATE(InEnd <= m_Data + m_Capacity);

			// Move each object in the range to the destination
			const SizeType Count = InternalDistance(InBegin, InEnd);
			if constexpr (std::is_trivially_move_assignable<T>())
			{
				const SizeType CpySize = Count * sizeof(T);
				memmove(Dest, InBegin, CpySize); // Assumes that data can overlap
			}
			else
			{
				while (InBegin != InEnd)
				{
					if constexpr (std::is_move_assignable<T>())
					{
						(*Dest) = Move(*InBegin);
					}
					else if constexpr (std::is_copy_assignable<T>())
					{
						(*Dest) = (*InBegin);
					}

					Dest++;
					InBegin++;
				}
			}
		}

		FORCEINLINE void InternalMemmoveForward(T* InBegin, T* InEnd, T* Dest)
		{
			// Move each object in the range to the destination, starts in the "End" and moves forward
			const SizeType Count = InternalDistance(InBegin, InEnd);
			if constexpr (std::is_trivially_move_assignable<T>())
			{
				if (Count > 0)
				{
					const SizeType CpySize = Count * sizeof(T);
					const SizeType OffsetSize = (Count - 1) * sizeof(T);
					memmove(reinterpret_cast<char*>(Dest) - OffsetSize, InBegin, CpySize);
				}
			}
			else
			{
				while (InEnd != InBegin)
				{
					InEnd--;
					if constexpr (std::is_move_assignable<T>())
					{
						(*Dest) = Move(*InEnd);
					}
					else if constexpr (std::is_copy_assignable<T>())
					{
						(*Dest) = (*InEnd);
					}
					Dest--;
				}
			}
		}

		FORCEINLINE void InternalDestruct(const T* Pos)
		{
			// Calls the destructor (If it needs to be called)
			if constexpr (std::is_trivially_destructible<T>() == false)
			{
				(*Pos).~T();
			}
		}

		FORCEINLINE void InternalDestructRange(const T* InBegin, const T* InEnd)
		{
			VALIDATE(InBegin <= InEnd);

			// Calls the destructor for every object in the range (If it needs to be called)
			if constexpr (std::is_trivially_destructible<T>() == false)
			{
				while (InBegin != InEnd)
				{
					InternalDestruct(InBegin);
					InBegin++;
				}
			}
		}

		FORCEINLINE void InternalDefaultConstructRange(T* InBegin, T* InEnd)
		{
			VALIDATE(InBegin <= InEnd);

			// Calls the default constructor for every object in the range (If it can be called)
			if constexpr (std::is_default_constructible<T>())
			{
				while (InBegin != InEnd)
				{
					new(reinterpret_cast<void*>(InBegin)) T();
					InBegin++;
				}
			}
		}

		T* m_Data;
		SizeType m_Size;
		SizeType m_Capacity;
	};
}