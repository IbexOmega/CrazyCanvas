#include "Networking/API/NetworkChallenge.h"

namespace LambdaEngine
{
	uint64 NetworkChallenge::Compute(uint64 clientSalt, uint64 serverSalt)
	{
		return clientSalt & serverSalt;
	}
}