#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformApplication.h"

namespace LambdaEngine
{
	CommonApplication* CommonApplication::s_pCommonApplication = nullptr;

	CommonApplication::CommonApplication()
	{
		VALIDATE(s_pCommonApplication == nullptr);
		s_pCommonApplication = this;
	}

	CommonApplication::~CommonApplication()
	{
		VALIDATE(s_pCommonApplication != nullptr);
		s_pCommonApplication = nullptr;
	}

	bool CommonApplication::Create()
	{
		m_pPlatformApplication = PlatformApplication::CreateApplication();
		if (m_pPlatformApplication->)
		return true;
	}

	

	bool CommonApplication::PreInit()
	{
		CommonApplication* pApplication = DBG_NEW CommonApplication();
		if (pApplication->Create())
		{
			return false;
		}
		
		return true;
	}

	bool CommonApplication::Tick()
	{
		PlatformApplication::ProcessMessages();
		
		CommonApplication::Get()->
		return true;
	}

	bool CommonApplication::PostRelease()
	{
		SAFEDELETE(s_pCommonApplication);
		return true;
	}
}
