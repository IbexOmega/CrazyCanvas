#pragma once
#include "Defines.h"
#include "Types.h"

#include "Assert/Assert.h"

namespace LambdaEngine
{
	// Removes reference retrives the type
	template<typename T>
	struct _TRemoveReference
	{
		using TType = T;
		using TConstRefType = const T;
	};

	template<typename T>
	struct _TRemoveReference<T&>
	{
		using TType = T;
		using TConstRefType = const T&;
	};

	template<typename T>
	struct _TRemoveReference<T&&>
	{
		using TType = T;
		using TConstRefType = const T&&;
	};

	template<typename T>
	using TRemoveReference = typename _TRemoveReference<T>::TType;

	// Removes pointer and retrives the type
	template<typename T>
	struct _TRemovePointer
	{
		using TType = T;
	};

	template<typename T>
	struct _TRemovePointer<T*>
	{
		using TType = T;
	};

	template<typename T>
	struct _TRemovePointer<T* const>
	{
		using TType = T;
	};

	template<typename T>
	struct _TRemovePointer<T* volatile>
	{
		using TType = T;
	};

	template<typename T>
	struct _TRemovePointer<T* const volatile>
	{
		using TType = T;
	};

	template<typename T>
	using TRemovePointer = typename _TRemovePointer<T>::TType;

	// Removes array type
	template<typename T>
	struct _TRemoveExtent
	{
		using TType = T;
	};

	template<typename T>
	struct _TRemoveExtent<T[]>
	{
		using TType = T;
	};

	template<typename T, size_t SIZE>
	struct _TRemoveExtent<T[SIZE]>
	{
		using TType = T;
	};

	template<typename T>
	using TRemoveExtent = typename _TRemoveExtent<T>::TType;

	// Move an object by converting it into a rvalue
	template<typename T>
	constexpr TRemoveReference<T>&& Move(T&& Object) noexcept
	{
		return static_cast<TRemoveReference<T>&&>(Object);
	}

	// Forward an object by converting it into a rvalue from an lvalue
	template<typename T>
	constexpr T&& Forward(TRemoveReference<T>& Arg) noexcept
	{
		return static_cast<T&&>(Arg);
	}

	// Forward an object by converting it into a rvalue from an rvalue
	template<typename T>
	constexpr T&& Forward(TRemoveReference<T>&& Arg) noexcept
	{
		return static_cast<T&&>(Arg);
	}
}