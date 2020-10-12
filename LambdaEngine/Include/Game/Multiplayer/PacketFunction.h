#pragma once

namespace LambdaEngine
{
	class IClient;
	class NetworkSegment;

	typedef std::function<void(IClient*, NetworkSegment*)> PacketFunction;
}
