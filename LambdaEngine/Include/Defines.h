#pragma once

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

//Helper macros
#define ZERO_MEMORY(memory, size) memset(memory, 0, size)
