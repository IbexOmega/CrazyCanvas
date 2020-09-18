#pragma once
#include "Event.h"

#include "Input/API/InputCodes.h"

namespace LambdaEngine
{
	/*
	* ShaderRecompileEvent
	*/
	struct ShaderRecompileEvent : public Event
	{
	public:
		inline ShaderRecompileEvent()
			: Event()
		{
		}

		DECLARE_EVENT_TYPE(ShaderRecompileEvent);

		virtual String ToString() const
		{
			return String("ShaderRecompileEvent");
		}

	};

	/*
	* PipelineStateRecompileEvent
	*/
	struct PipelineStateRecompileEvent : public Event
	{
	public:
		inline PipelineStateRecompileEvent()
			: Event()
		{
		}

		DECLARE_EVENT_TYPE(PipelineStateRecompileEvent);

		virtual String ToString() const
		{
			return String("PipelineStateRecompileEvent");
		}

	};

	/*
	* PipelineStateRecompileEvent
	*/
	struct PipelineStatesRecompiledEvent : public Event
	{
	public:
		inline PipelineStatesRecompiledEvent()
			: Event()
		{
		}

		DECLARE_EVENT_TYPE(PipelineStatesRecompiledEvent);

		virtual String ToString() const
		{
			return String("PipelineStatesRecompiledEvent");
		}

	};
}