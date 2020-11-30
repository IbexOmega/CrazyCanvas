#pragma once
#include "Log/Log.h"

#include "Physics/PhysX/PhysX.h"

namespace LambdaEngine
{
	class PhysXErrorCallback : public physx::PxErrorCallback
	{
	public:
		PhysXErrorCallback() = default;
		~PhysXErrorCallback() = default;

		inline virtual void reportError(physx::PxErrorCode::Enum code, const char* pMessage, const char* pFile, int lineNr) override final
		{
			ELogSeverity severity;
			switch (code)
			{
				case physx::PxErrorCode::eNO_ERROR:
				case physx::PxErrorCode::eDEBUG_INFO:
					severity = ELogSeverity::LOG_INFO;
					break;
				case physx::PxErrorCode::eDEBUG_WARNING:
				case physx::PxErrorCode::ePERF_WARNING:
					severity = ELogSeverity::LOG_WARNING;
					break;
				default:
					severity = ELogSeverity::LOG_ERROR;
					break;
			}

			LOG(pFile, lineNr, severity, pMessage);
		}
	};
};
