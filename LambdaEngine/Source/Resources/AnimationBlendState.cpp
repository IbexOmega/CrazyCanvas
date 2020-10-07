#include "Resources/AnimationBlendState.h"

namespace LambdaEngine
{
	/*
	* AnimationBlendState
	*/

	AnimationBlendState::AnimationBlendState()
		: m_Name()
		, m_TotalWeight(0.0f)
		, m_BlendInfos()
	{
	}

	void AnimationBlendState::PushBlendInfo(const AnimationBlendInfo& animationBlendInfo, bool normalizeWeights)
	{
		if (m_BlendInfos.GetSize() == 2)
		{
			LOG_WARNING("[AnimationBlendState]: For now only two animations can blend at one time, latest blendinfo is NOT added");
			return;
		}

		m_BlendInfos.PushBack(animationBlendInfo);
		if (normalizeWeights)
		{
			CalculateWeights();
		}
	}
	
	void AnimationBlendState::PushBlendInfo(AnimationBlendInfo&& animationBlendInfo, bool normalizeWeights)
	{
		if (m_BlendInfos.GetSize() == 2)
		{
			LOG_WARNING("[AnimationBlendState]: For now only two animations can blend at one time, latest blendinfo is NOT added");
			return;
		}

		m_BlendInfos.PushBack(animationBlendInfo);
		if (normalizeWeights)
		{
			CalculateWeights();
		}
	}

	void AnimationBlendState::PopBlendInfo(bool normalizeWeights)
	{
		if (!m_BlendInfos.IsEmpty())
		{
			m_BlendInfos.PopBack();

			if (normalizeWeights)
			{
				CalculateWeights();
			}
		}
	}

	void AnimationBlendState::CalculateWeights()
	{
		float32 totalWeight = 0.0f;
		for (AnimationBlendInfo& info : m_BlendInfos)
		{
			info.StaticWeight = fabs(info.StaticWeight);
			totalWeight += info.StaticWeight;
		}

		// Protect against divide by zero
		if (totalWeight > 0.0f)
		{
			for (AnimationBlendInfo& info : m_BlendInfos)
			{
				info.NormalizedWeight = info.StaticWeight / totalWeight;
			}

			m_TotalWeight = totalWeight;
		}
	}

	/*
	* AnimationState
	*/

	AnimationState::AnimationState()
		: m_Clips()
		, m_BlendStates()
		, m_CurrentBlendStateIndex(0)
		, m_TotalWeight(0.0f)
	{
	}

	AnimationState::AnimationState(const ClipState& clipState)
		: m_Clips()
		, m_BlendStates()
		, m_CurrentBlendStateIndex(0)
		, m_TotalWeight(0.0f)
	{
		PushClip(clipState);

		AnimationBlendState blendState;
		blendState.PushBlendInfo(AnimationBlendInfo(clipState.Name, 1.0f));
		PushBlendState(blendState);
	}

	void AnimationState::PushClip(const ClipState& clipState)
	{
		if (!HasClip(clipState.Name))
		{
			m_Clips.PushBack(clipState);
		}
	}

	void AnimationState::PushClip(ClipState&& clipState)
	{
		if (!HasClip(clipState.Name))
		{
			m_Clips.PushBack(clipState);
		}
	}

	void AnimationState::PopClip()
	{
		if (!m_Clips.IsEmpty())
		{
			m_Clips.PopBack();
		}
	}

	void AnimationState::PushBlendState(const AnimationBlendState& blendState)
	{
		if (!HasBlendState(blendState.GetName()))
		{
			m_BlendStates.PushBack(blendState);
		}
	}

	void AnimationState::PushBlendState(AnimationBlendState&& blendState)
	{
		if (!HasBlendState(blendState.GetName()))
		{
			m_BlendStates.PushBack(blendState);
		}
	}

	void AnimationState::PopBlendState()
	{
		if (!m_BlendStates.IsEmpty())
		{
			m_BlendStates.PopBack();
		}
	}

	bool AnimationState::HasClip(const String& name) const
	{
		for (const ClipState& info : m_Clips)
		{
			if (info.Name == name)
			{
				return true;
			}
		}

		return false;
	}

	bool AnimationState::HasBlendState(const String& name) const
	{
		for (const AnimationBlendState& blendState : m_BlendStates)
		{
			if (blendState.GetName() == name)
			{
				return true;
			}
		}

		return false;
	}

	ClipState& AnimationState::GetClip(const String& name)
	{
		// Only a few animations in here, is probably faster this way compared to hashing
		for (ClipState& info : m_Clips)
		{
			if (info.Name == name)
			{
				return info;
			}
		}

		// Should not reach here
		VALIDATE(0);
		return m_Clips[0];
	}

	const ClipState& AnimationState::GetClip(const String& name) const
	{
		// Only a few animations in here, is probably faster this way compared to hashing
		for (const ClipState& info : m_Clips)
		{
			if (info.Name == name)
			{
				return info;
			}
		}

		// Should not reach here
		VALIDATE(0);
		return m_Clips[0];
	}

	AnimationBlendState& AnimationState::GetBlendState(const String& name)
	{
		// Only a few blendstates in here, is probably faster this way compared to hashing
		for (AnimationBlendState& blendState : m_BlendStates)
		{
			if (blendState.GetName() == name)
			{
				return blendState;
			}
		}

		// Should not reach here
		VALIDATE(0);
		return m_BlendStates[0];
	}

	const AnimationBlendState& AnimationState::GetBlendState(const String& name) const
	{
		// Only a few blendstates in here, is probably faster this way compared to hashing
		for (const AnimationBlendState& blendState : m_BlendStates)
		{
			if (blendState.GetName() == name)
			{
				return blendState;
			}
		}

		// Should not reach here
		VALIDATE(0);
		return m_BlendStates[0];
	}

	AnimationBlendState& AnimationState::GetCurrentBlendState()
	{
		VALIDATE(m_BlendStates.IsEmpty() == false);
		return m_BlendStates[m_CurrentBlendStateIndex];
	}

	const AnimationBlendState& AnimationState::GetCurrentBlendState() const
	{
		VALIDATE(m_BlendStates.IsEmpty() == false);
		return m_BlendStates[m_CurrentBlendStateIndex];
	}
}
