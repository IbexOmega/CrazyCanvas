#pragma once
#include "Event.h"

#include "Utilities/HashUtilities.h"

namespace LambdaEngine
{
	/*
	* IEventHandler
	*/
	
	class IEventHandler
	{
	public:
		virtual ~IEventHandler() = default;

		virtual bool Execute(const Event& event) = 0;
		
		virtual uint64 GetHash() const		= 0;
		virtual const void*	GetPtr() const	= 0;
	};

	/*
	* FunctionEventHandler
	*/

	template<typename TEvent>
	class FunctionEventHandler : public IEventHandler
	{
		typedef bool(*Func)(const TEvent&);
	
	public:
		inline explicit FunctionEventHandler(Func pFunc)
			: IEventHandler()
			, m_pFunc(pFunc)
		{
		}

		virtual bool Execute(const Event& event) override final
		{
			return m_pFunc(static_cast<const TEvent&>(event));
		}

		virtual uint64 GetHash() const override final
		{
			std::hash<const void*> hasher;
			return hasher(reinterpret_cast<const void*>(m_pFunc));
		}

		virtual const void* GetPtr() const override final
		{
			return reinterpret_cast<const void*>(m_pFunc);
		}

	private:
		Func m_pFunc = nullptr;
	};

	/*
	* MemberEventHandler
	*/

	template<typename T, typename TEvent>
	class MemberEventHandler : public IEventHandler
	{
		typedef bool(T::*MemberFunc)(const TEvent&);
	
	public:
		inline explicit MemberEventHandler(T* pThis, MemberFunc pFunc)
			: IEventHandler()
			, m_pThis(pThis)
			, m_pFunc(pFunc)
		{
		}

		virtual bool Execute(const Event& event) override final
		{
			VALIDATE(m_pThis != nullptr);
			return ((*m_pThis).*(m_pFunc))(static_cast<const TEvent&>(event));
		}

		virtual uint64 GetHash() const override final
		{
			size_t hash = typeid(MemberFunc).hash_code();
			return HashCombine(hash, m_pThis);
		}

		virtual const void* GetPtr() const override final
		{
			return reinterpret_cast<const void*>(m_pThis);
		}

	private:
		MemberFunc m_pFunc = nullptr;
		T* m_pThis = nullptr;
	};

	/*
	* EventHandler
	*/

	class EventHandler
	{
	public:
		template<typename TEvent>
		inline EventHandler(bool(*pFunc)(const TEvent&)) noexcept
			: m_StackBuffer()
			, m_pEventHandler(nullptr)
		{
			constexpr auto stackSize	= sizeof(m_StackBuffer);
			constexpr auto handlerSize	= sizeof(FunctionEventHandler<TEvent>);
			static_assert(handlerSize <= stackSize);

			ZERO_MEMORY(m_StackBuffer, stackSize);

			// Placement new is needed to fully initialize vtable
			new(reinterpret_cast<void*>(m_StackBuffer)) FunctionEventHandler<TEvent>(pFunc);
			m_pEventHandler = reinterpret_cast<IEventHandler*>(m_StackBuffer);
		}

		template<typename T, typename TEvent>
		inline EventHandler(T* pThis, bool(T::*pMemberFunc)(const TEvent&)) noexcept
			: m_StackBuffer()
			, m_pEventHandler(nullptr)
		{
			constexpr auto stackSize	= sizeof(m_StackBuffer);
			constexpr auto handlerSize	= sizeof(MemberEventHandler<T, TEvent>);
			static_assert(handlerSize <= stackSize);

			ZERO_MEMORY(m_StackBuffer, stackSize);

			// Placement new is needed to fully initialize vtable
			new(reinterpret_cast<void*>(m_StackBuffer)) MemberEventHandler<T, TEvent>(pThis, pMemberFunc);
			m_pEventHandler = reinterpret_cast<IEventHandler*>(m_StackBuffer);
		}

		inline EventHandler(const EventHandler& other) noexcept
			: m_StackBuffer()
			, m_pEventHandler(nullptr)
		{
			/*
				Copy all the memory. Should work since we now basically have a copy of the object and the objects are only data
				This should not be used as a general solution if an object requires its move/copy constructors. However, 
				in this context it works since we know how the objects look
			*/ 
			memcpy(m_StackBuffer, other.m_StackBuffer, sizeof(m_StackBuffer));
			m_pEventHandler = reinterpret_cast<IEventHandler*>(m_StackBuffer);
		}

		inline ~EventHandler()
		{
			if (m_pEventHandler)
			{
				m_pEventHandler->~IEventHandler();
			}
		}

		FORCEINLINE bool Call(const Event& event) const
		{
			VALIDATE(m_pEventHandler != nullptr);
			return m_pEventHandler->Execute(event);
		}

		FORCEINLINE EventHandler& operator=(const EventHandler& other)
		{
			if (this != std::addressof(other))
			{
				memcpy(m_StackBuffer, other.m_StackBuffer, sizeof(m_StackBuffer));
				m_pEventHandler = reinterpret_cast<IEventHandler*>(m_StackBuffer);
			}

			return *this;
		}

		FORCEINLINE bool operator()(const Event& event) const
		{
			return Call(event);
		}

		FORCEINLINE bool operator==(const EventHandler& other) const
		{
			const uint64 hash0 = m_pEventHandler->GetHash();
			const uint64 hash1 = other.m_pEventHandler->GetHash();
			
			if (hash0 == hash1)
			{
				const void* pPtr0 = m_pEventHandler->GetPtr();
				const void* pPtr1 = other.m_pEventHandler->GetPtr();
				return pPtr0 == pPtr1;
			}
			else
			{
				return false;
			}
		}

	private:
		// Size of four pointers, this is incase we use the memberfunctionhandler
		//		1 ptr - vtable
		//		1 ptr - functionptr
		//		1 extra ptr - This is the case when the T* variable is of a type that uses multiple inheritence. Then the member function
		//			pointer does not only store a pointer to a function but also an adjustor for adjusting the this pointer when making a call.
		//		1 ptr - this (for memberfunctionhandler)
		byte m_StackBuffer[sizeof(void*) * 4];
		IEventHandler* m_pEventHandler;
	};
}