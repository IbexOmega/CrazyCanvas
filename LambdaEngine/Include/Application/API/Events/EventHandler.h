#pragma once
#include "Event.h"

namespace LambdaEngine
{
	/*
	* EventHandler
	*/
	class EventHandler
	{
	public:
		inline virtual bool Execute(const Event& event) = 0;
	};

	/*
	* MemberEventHandler
	*/
	template<typename T>
	class MemberEventHandler
	{
	public:
		inline MemberEventHandler()
			: EventHandler()
			, 
		{
		}

		inline virtual bool Execute(const Event& event) override final
		{
			return m_pThis->m_pFunc(event);
		}

	private:
		T* m_pThis		= nullptr;
		T::* m_pFunc	= nullptr;
	};

	/*
	* EventHandlerProxy
	*/
	class EventHandlerProxy
	{
	public:
		bool operator()(const Event& event)
		{
			return m_pEventHandler->Execute(event);
		}

	private:
		EventHandler* m_pEventHandler;
	};
}