#include "Rendering/DrawArgHelper.h"

using namespace LambdaEngine;

THashTable<const ComponentType*, uint32> DrawArgHelper::s_ComponentToDrawArgMask;

uint32 DrawArgHelper::FetchComponentDrawArgMask(const ComponentType* type)
{
	static uint32 s_Counter = 0;
	auto it = s_ComponentToDrawArgMask.find(type);
	if (it == s_ComponentToDrawArgMask.end())
		return s_ComponentToDrawArgMask[type] = (1 << s_Counter);
	return it->second;
}