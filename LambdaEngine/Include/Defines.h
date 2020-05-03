#pragma once

/*
* Configuration
*/
#if defined(LAMBDA_CONFIG_DEBUG)
	#define LAMBDA_DEBUG
#elif defined(LAMBDA_CONFIG_RELEASE)
	#define LAMBDA_RELEASE
#elif defined(LAMBDA_CONFIG_PRODUCTION)
	#define LAMBDA_PRODUCTION
#endif

#if defined(LAMBDA_DEBUG) || defined(LAMBDA_RELEASE)
	#define LAMBDA_DEVELOPMENT
#endif

/*
* Exporting dynamic libraries
*/ 
#if defined(LAMBDA_PLATFORM_WINDOWS) && defined(LAMBDA_SHARED_LIB) 
	#ifdef LAMBDA_EXPORT	
		#define LAMBDA_API _declspec(dllexport)
	#else
		#define LAMBDA_API _declspec(dllimport)
	#endif
#else
	#define LAMBDA_API
#endif

/*
* Unused params
*/ 
#define UNREFERENCED_VARIABLE(variable) (void)variable

/*
* Declaration helpers
*/ 
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

#define DECL_UNIQUE_CLASS(Typename) \
	DECL_REMOVE_COPY(Typename); \
	DECL_REMOVE_MOVE(Typename); \

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

/*
* Helper Macros
*/ 
#define ZERO_MEMORY(memory, size)	memset((void*)memory, 0, size)
#define ARR_SIZE(arr)				sizeof(arr) / sizeof(arr[0])

/*
* Forceinline
*/ 
#ifdef LAMBDA_VISUAL_STUDIO
	#define FORCEINLINE __forceinline
#else
	#define FORCEINLINE __attribute__((always_inline)) inline
#endif

/*
* Delete and Release
*/ 
#define DELETE_OBJECT(object)	delete (object); (object) = nullptr
#define SAFEDELETE(object)		if ((object))	{ DELETE_OBJECT(object); }

#define DELETE_ARRAY(array)		delete[] (array); (array) = nullptr
#define SAFEDELETE_ARRAY(array)	if ((array))	{ DELETE_ARRAY(array); }

#define RELEASE(object)			(object)->Release(); (object) = nullptr
#define SAFERELEASE(object)		if ((object))	{ RELEASE(object); }

/*
* Bit-Mask helpers
*/ 
#define BIT(bit)	(1 << bit)
#define FLAG(bit)	BIT(bit)

/*
* String preprocessor handling
*   There are two versions of STRING_CONCAT, this is so that you can use __LINE__, __FILE__ etc. within the macro,
*   therefore always use STRING_CONCAT
*/ 
#define _STRING_CONCAT(x, y) x##y
#define STRING_CONCAT(x, y) _STRING_CONCAT(x, y)

/*
 * Helpers for size
 */
#define MEGA_BYTE(megabytes) (megabytes) * 1024 * 1024
