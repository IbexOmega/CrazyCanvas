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

#define DECL_INTERFACE(Typename) DECL_ABSTRACT_CLASS(Typename)

//Helper macros
#define ZERO_MEMORY(memory, size) memset((void*)memory, 0, size)
#define ARR_SIZE(arr) sizeof(arr) / sizeof(arr[0])

//Inline
#ifdef LAMBDA_VISUAL_STUDIO
	#define FORCEINLINE __forceinline
#else
	#define FORCEINLINE __attribute__((always_inline))
#endif

//Delete and release
#define SAFEDELETE(object)	if ((object)) { delete object; object = nullptr; }
#define SAFERELEASE(object) if ((object)) { object->Release(); object = nullptr; }

//Bit helper
#define BIT(bit) (1 << bit)
