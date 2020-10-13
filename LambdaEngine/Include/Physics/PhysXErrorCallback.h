#pragma once

#include "Log/Log.h"

#include <foundation/PxErrorCallback.h>

namespace LambdaEngine
{
	class PhysXErrorCallback : public physx::PxErrorCallback
	{
	public:
		PhysXErrorCallback() = default;
		~PhysXErrorCallback() = default;

		inline void reportError(physx::PxErrorCode::Enum code, const char* pMessage, const char* pFile, int line) override final
		{
			UNREFERENCED_VARIABLE(pFile);
			UNREFERENCED_VARIABLE(line);

			using namespace physx;
			switch (code)
			{
				case PxErrorCode::eNO_ERROR:
				case PxErrorCode::eDEBUG_INFO:
					D_LOG_INFO(pMessage);
					break;
				case PxErrorCode::eDEBUG_WARNING:
				case PxErrorCode::ePERF_WARNING:
					LOG_WARNING(pMessage);
					break;
				default:
					LOG_ERROR(pMessage);
					break;
			}
		}
	};
};
