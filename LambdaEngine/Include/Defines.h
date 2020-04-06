#pragma once
#include <string.h>

// Exporting
#if defined(LAMBDA_PLATFORM_WINDOWS) && defined(LAMBDA_SHARED_LIB) 
	#ifdef LAMBDA_EXPORT	
		#define LAMBDA_API _declspec(dllexport)
	#else
		#define LAMBDA_API _declspec(dllimport)
	#endif
#else
	#define LAMBDA_API
#endif

//Unused params
#define UNREFERENCED_VARIABLE(variable) variable

//Declaration helpers
#define DECL_REMOVE_COPY(Typename) \
	Typename(const Typename&) = delete; \
	Typename& operator=(const Typename&) = delete \

#define DECL_REMOVE_MOVE(Typename) \
	Typename(Typename&&) = delete; \
	Typename& operator=(Typename&&) = delete \

#define DECL_STATIC_CLASS(Typename) \
	DECL_REMOVE_COPY(Typename); \
	DECL_REMOVE_MOVE(Typename); \
	Typename() = delete; \
	~Typename() = delete

#define DECL_ABSTRACT_CLASS(Typename) \
	DECL_REMOVE_COPY(Typename); \
	DECL_REMOVE_MOVE(Typename); \
	Typename() = default; \
	virtual ~Typename() = default

/*
* Difference between this and the other DECL_INTERFACE is that this does not have a virtual destructor.
* This is desired since we want Release to be used.
*/
#define DECL_DEVICE_INTERFACE(Typename) \
	DECL_REMOVE_COPY(Typename);			\
	DECL_REMOVE_MOVE(Typename);			\
	Typename() = default; \
	~Typename() = default

#define DECL_INTERFACE(Typename) DECL_ABSTRACT_CLASS(Typename)

//Helper macros
#define ZERO_MEMORY(memory, size) memset((void*)memory, 0, size)
#define ARR_SIZE(arr) sizeof(arr) / sizeof(arr[0])

//Inline
#ifdef LAMBDA_VISUAL_STUDIO
	#define FORCEINLINE __forceinline
#else
	#define FORCEINLINE __attribute__((always_inline)) inline
#endif

//Delete and release
#define SAFEDELETE(object)		if ((object))	{ delete object; object = nullptr; }
#define SAFEDELETEARR(array)	if ((array))	{ delete[] array; array = nullptr; }
#define SAFERELEASE(object)		if ((object))	{ object->Release(); object = nullptr; }

//Bit-Mask helpers
#define BIT(bit)	(1 << bit)
#define FLAG(bit)	BIT(bit)
