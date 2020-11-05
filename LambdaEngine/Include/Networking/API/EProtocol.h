#pragma once

#include "LambdaEngine.h"

#include "Containers/String.h"

namespace LambdaEngine
{
	enum EProtocol
	{
		TCP,
		UDP
	};

	class EProtocolParser
	{
		DECL_STATIC_CLASS(EProtocolParser);

	public:
		static String ToString(EProtocol protocol);
		static EProtocol FromString(const String& string);
	};
}