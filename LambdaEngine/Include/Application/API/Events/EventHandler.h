#pragma once
#include "Event.h"

namespace LambdaEngine
{
	/*
	* IEventHandler
	*/
	class IEventHandler
	{
	public:
		virtual bool Execute(const Event& event) = 0;
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

		inline virtual bool Execute(const Event& event) override final
		{
			return m_pFunc(static_cast<const TEvent&>(event));
		}

	private:
		Func m_pFunc;
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

		inline virtual bool Execute(const Event& event) override final
		{
			VALIDATE(m_pThis != nullptr);
			return ((*m_pThis).*(m_pFunc))(static_cast<const TEvent&>(event));
		}

	private:
		MemberFunc m_pFunc = nullptr;
		T* m_pThis = nullptr;
	};

	/*
	* EventHandlerProxy
	*/
	class EventHandlerProxy
	{
	public:
		template<typename TEvent>
		inline EventHandlerProxy(bool(*pFunc)(const TEvent&)) noexcept
			: m_StackBuffer()
			, m_pEventHandler(nullptr)
		{
			// Placement new is needed to fully initialize vtable
			new(reinterpret_cast<void*>(m_StackBuffer)) FunctionEventHandler<TEvent>(pFunc);
			m_pEventHandler = reinterpret_cast<IEventHandler*>(m_StackBuffer);
		}

		template<typename T, typename TEvent>
		inline EventHandlerProxy(T* pThis, bool(T::*pMemberFunc)(const TEvent&)) noexcept
			: m_StackBuffer()
			, m_pEventHandler(nullptr)
		{
			// Placement new is needed to fully initialize vtable
			new(reinterpret_cast<void*>(m_StackBuffer)) MemberEventHandler<T, TEvent>(pThis, pMemberFunc);
			m_pEventHandler = reinterpret_cast<IEventHandler*>(m_StackBuffer);
		}

		inline EventHandlerProxy(const EventHandlerProxy& other) noexcept
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

		inline bool Call(const Event& event) const
		{
			VALIDATE(m_pEventHandler != nullptr);
			return m_pEventHandler->Execute(event);
		}

		inline bool operator()(const Event& event) const
		{
			return Call(event);
		}

		EventHandlerProxy& operator=(const EventHandlerProxy& other)
		{
			if (this != std::addressof(other))
			{
				memcpy(m_StackBuffer, other.m_StackBuffer, sizeof(m_StackBuffer));
				m_pEventHandler = reinterpret_cast<IEventHandler*>(m_StackBuffer);
			}

			return *this;
		}

	private:
		// Size of three pointers, this is incase we use the memberfunctionhandler
		//		1 ptr - vtable
		//		1 ptr - functionptr
		//		1 ptr - this (for memberfunctionhandler)
		byte m_StackBuffer[sizeof(void*) * 3];
		IEventHandler* m_pEventHandler;
	};
}