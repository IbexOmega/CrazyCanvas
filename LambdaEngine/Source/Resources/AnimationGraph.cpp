#include "Resources/AnimationGraph.h"
#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	/*
	* BinaryInterpolator
	*/

	void BinaryInterpolator::Interpolate(float32 factor)
	{
		VALIDATE(In0.GetSize() == In1.GetSize());
		
		const uint32 size = In0.GetSize();
		const float32 realFactor = glm::clamp(factor, 0.0f, 1.0f);

		if (Out.GetSize() < size)
		{
			Out.Resize(size);
		}

		for (uint32 i = 0; i < size; i++)
		{
			const SQT& in0	= In0[i];
			const SQT& in1	= In1[i];
			SQT& out		= Out[i];

			out.Translation	= glm::mix(in0.Translation, in1.Translation,	realFactor);
			out.Scale		= glm::mix(in0.Scale,		in1.Scale,			realFactor);
			out.Rotation	= glm::slerp(in0.Rotation,	in1.Rotation,		realFactor);
			out.Rotation	= glm::normalize(out.Rotation);
		}
	}

	/*
	* Transition
	*/

	Transition::Transition(const String& fromState, const String& toState, float64 fromBeginAt, float64 toBeginAt)
		: m_pOwnerGraph(nullptr)
		, m_From(UINT32_MAX)
		, m_To(UINT32_MAX)
		, m_IsActive(false)
		, m_FromState(fromState)
		, m_FromBeginAt(fromBeginAt)
		, m_ToBeginAt(toBeginAt)
		, m_LocalClock(0.0f)
		, m_ToState(toState)
	{
		Reset();
	}

	void Transition::Tick(const float64 delta)
	{
		UNREFERENCED_VARIABLE(delta);

		VALIDATE(m_From != UINT32_MAX);

		// Only transition if the clip has reached correct timing
		AnimationState& fromState = m_pOwnerGraph->GetState(m_From);
		const float64 normalizedTime = fromState.GetNormlizedTime();

		//LOG_INFO("normalizedTime=%.4f, m_FromBeginAt=%.4f", normalizedTime, m_FromBeginAt);

		if (normalizedTime >= m_FromBeginAt)
		{
			// This makes sure that we loop around one time if we start a transition after the sync-point
			if (m_IsActive)
			{
				m_LocalClock = normalizedTime;
			}
		}
		else
		{
			m_IsActive = true;
		}

		// Calculate deltatime, used for determine if we should finish transition
		if (m_LastTime > 0.0f)
		{
			m_DeltaTime = normalizedTime - m_LastTime;
		}

		m_LastTime = normalizedTime;
	}

	bool Transition::Equals(const String& fromState, const String& toState) const
	{
		return m_FromState == fromState && m_ToState == toState;
	}

	bool Transition::UsesState(const String& state) const
	{
		return m_FromState == state || m_ToState == state;
	}

	void Transition::SetAnimationGraph(AnimationGraph* pGraph)
	{
		VALIDATE(pGraph != nullptr);

		m_pOwnerGraph = pGraph;
		m_From	= m_pOwnerGraph->GetStateIndex(m_FromState);
		m_To	= m_pOwnerGraph->GetStateIndex(m_ToState);
	}

	/*
	* AnimationState
	*/

	AnimationState::AnimationState()
		: m_pOwnerGraph(nullptr)
		, m_IsLooping(true)
		, m_NumLoops(INFINITE_LOOPS)
		, m_StartTime(0.0)
		, m_RunningTime(0.0)
		, m_PlaybackSpeed(1.0)
		, m_NormalizedTime(0.0)
		, m_LocalTimeInSeconds(0.0)
		, m_DurationInSeconds(0.0)
		, m_Name()
		, m_AnimationGUID(GUID_NONE)
	{
	}

	AnimationState::AnimationState(const String& name, GUID_Lambda animationGUID, bool isLooping)
		: m_pOwnerGraph(nullptr)
		, m_IsLooping(isLooping)
		, m_NumLoops(1)
		, m_StartTime(0.0)
		, m_RunningTime(0.0)
		, m_PlaybackSpeed(1.0)
		, m_NormalizedTime(0.0)
		, m_LocalTimeInSeconds(0.0)
		, m_DurationInSeconds(0.0)
		, m_Name(name)
		, m_AnimationGUID(animationGUID)
	{
		Animation* pAnimation = ResourceManager::GetAnimation(animationGUID);
		if (pAnimation)
		{
			m_DurationInSeconds = pAnimation->DurationInSeconds();
		}

		if (m_IsLooping)
		{
			m_NumLoops = INFINITE_LOOPS;
		}
	}

	void AnimationState::Tick(const float64 deltaTime)
	{
		// Get localtime for the animation-clip
		m_RunningTime += deltaTime;
		
		float64 localTime = m_RunningTime * fabs(m_PlaybackSpeed);
		if (m_IsLooping)
		{
			if (m_NumLoops != INFINITE_LOOPS)
			{
				const float64 totalDuration = m_NumLoops * GetDurationInSeconds();
				localTime = glm::clamp(localTime, 0.0, totalDuration);
			}

			localTime = fmod(localTime, GetDurationInSeconds());
		}
		else
		{
			localTime = glm::clamp(localTime, 0.0, GetDurationInSeconds());
		}

		m_LocalTimeInSeconds	= localTime;
		m_NormalizedTime		= m_LocalTimeInSeconds / m_DurationInSeconds;
		if (m_PlaybackSpeed < 0.0)
		{
			m_NormalizedTime = 1.0 - m_NormalizedTime;
		}
	}

	void AnimationState::Interpolate(const Skeleton& skeleton)
	{
		Animation& animation = GetAnimation();
		const float64 time = GetNormlizedTime() * animation.DurationInTicks;

		if (m_CurrentFrame.GetSize() < skeleton.Joints.GetSize())
		{
			m_CurrentFrame.Resize(skeleton.Joints.GetSize());
		}

		for (Animation::Channel& channel : animation.Channels)
		{
			// Retrive the bone ID
			auto it = skeleton.JointMap.find(channel.Name);
			if (it == skeleton.JointMap.end())
			{
				continue;
			}

			// Sample SQT for this animation
			glm::vec3 position	= SamplePosition(channel, time);
			glm::quat rotation	= SampleRotation(channel, time);
			glm::vec3 scale		= SampleScale(channel, time);

			const uint32 jointID = it->second;
			m_CurrentFrame[jointID] = SQT(position, scale, rotation);
		}
	}

	Animation& AnimationState::GetAnimation() const
	{
		Animation* pAnimation = ResourceManager::GetAnimation(m_AnimationGUID);
		VALIDATE(pAnimation != nullptr);
		return *pAnimation;
	}

	glm::vec3 AnimationState::SamplePosition(Animation::Channel& channel, float64 time)
	{
		// If the clip is looping the last frame is redundant
		const uint32 numPositions = m_IsLooping ? channel.Positions.GetSize() - 1 : channel.Positions.GetSize();

		Animation::Channel::KeyFrame pos0 = channel.Positions[0];
		Animation::Channel::KeyFrame pos1 = channel.Positions[0];
		if (numPositions > 1)
		{
			for (uint32 i = 0; i < (numPositions - 1); i++)
			{
				if (time < channel.Positions[i + 1].Time)
				{
					pos0 = channel.Positions[i];
					pos1 = channel.Positions[i + 1];
					break;
				}
			}
		}

		const float64 factor = (pos1.Time != pos0.Time) ? (time - pos0.Time) / (pos1.Time - pos0.Time) : 0.0f;
		glm::vec3 position = glm::mix(pos0.Value, pos1.Value, glm::vec3(factor));
		return position;
	}

	glm::vec3 AnimationState::SampleScale(Animation::Channel& channel, float64 time)
	{
		// If the clip is looping the last frame is redundant
		const uint32 numScales = m_IsLooping ? channel.Scales.GetSize() - 1 : channel.Scales.GetSize();

		Animation::Channel::KeyFrame scale0 = channel.Scales[0];
		Animation::Channel::KeyFrame scale1 = channel.Scales[0];
		if (numScales > 1)
		{
			for (uint32 i = 0; i < (numScales - 1); i++)
			{
				if (time < channel.Scales[i + 1].Time)
				{
					scale0 = channel.Scales[i];
					scale1 = channel.Scales[i + 1];
					break;
				}
			}
		}

		const float64 factor = (scale1.Time != scale0.Time) ? (time - scale0.Time) / (scale1.Time - scale0.Time) : 0.0f;
		glm::vec3 scale = glm::mix(scale0.Value, scale1.Value, glm::vec3(factor));
		return scale;
	}

	glm::quat AnimationState::SampleRotation(Animation::Channel& channel, float64 time)
	{
		// If the clip is looping the last frame is redundant
		const uint32 numRotations = m_IsLooping ? channel.Rotations.GetSize() - 1 : channel.Rotations.GetSize();

		Animation::Channel::RotationKeyFrame rot0 = channel.Rotations[0];
		Animation::Channel::RotationKeyFrame rot1 = channel.Rotations[0];
		if (numRotations > 1)
		{
			for (uint32 i = 0; i < (numRotations - 1); i++)
			{
				if (time < channel.Rotations[i + 1].Time)
				{
					rot0 = channel.Rotations[i];
					rot1 = channel.Rotations[i + 1];
					break;
				}
			}
		}

		const float64 factor = (rot1.Time != rot0.Time) ? (time - rot0.Time) / (rot1.Time - rot0.Time) : 0.0;
		glm::quat rotation = glm::slerp(rot0.Value, rot1.Value, float32(factor));
		rotation = glm::normalize(rotation);
		return rotation;
	}

	/*
	* AnimationGraph
	*/

	AnimationGraph::AnimationGraph()
		: m_IsBlending(false)
		, m_CurrentTransition(INVALID_TRANSITION)
		, m_CurrentState(0)
		, m_States()
		, m_Transitions()
		, m_TransitionResult()
	{
	}

	AnimationGraph::AnimationGraph(const AnimationState& animationState)
		: m_IsBlending(false)
		, m_CurrentTransition(INVALID_TRANSITION)
		, m_CurrentState(0)
		, m_States()
		, m_Transitions()
		, m_TransitionResult()
	{
		AddState(animationState);
	}

	AnimationGraph::AnimationGraph(AnimationState&& animationState)
		: m_IsBlending(false)
		, m_CurrentTransition(INVALID_TRANSITION)
		, m_CurrentState(0)
		, m_States()
		, m_Transitions()
		, m_TransitionResult()
	{
		AddState(animationState);
	}

	AnimationGraph::AnimationGraph(AnimationGraph&& other)
		: m_IsBlending(other.m_IsBlending)
		, m_CurrentTransition(other.m_CurrentTransition)
		, m_CurrentState(other.m_CurrentState)
		, m_States(std::move(other.m_States))
		, m_Transitions(std::move(other.m_Transitions))
		, m_TransitionResult(std::move(other.m_TransitionResult))
	{
		other.m_IsBlending			= false;
		other.m_CurrentTransition	= -1;
		other.m_CurrentState		= -1;

		SetOwnerGraph();
	}

	AnimationGraph::AnimationGraph(const AnimationGraph& other)
		: m_IsBlending(other.m_IsBlending)
		, m_CurrentTransition(other.m_CurrentTransition)
		, m_CurrentState(other.m_CurrentState)
		, m_States(other.m_States)
		, m_Transitions(other.m_Transitions)
		, m_TransitionResult(other.m_TransitionResult)
	{
		SetOwnerGraph();
	}

	void AnimationGraph::Tick(float64 deltaTimeInSeconds, float64 globalTimeInSeconds, const Skeleton& skeleton)
	{
		if (IsTransitioning())
		{
			AnimationState& fromState = GetCurrentState();
			fromState.Tick(deltaTimeInSeconds);
			fromState.Interpolate(skeleton);

			Transition& currentTransition = GetCurrentTransition();
			currentTransition.Tick(deltaTimeInSeconds);

			// LOG_INFO("Weight=%.4f, LocalTime=%.4f", currentTransition.GetWeight(), fromState.GetNormlizedTime());

			VALIDATE(HasState(currentTransition.To()));
			AnimationState& toState = GetState(currentTransition.To());
			if (currentTransition.GetWeight() > 0.0f)
			{
				if (!toState.IsPlaying())
				{
					toState.StartUp(globalTimeInSeconds, currentTransition.m_ToBeginAt);
				}

				toState.Tick(deltaTimeInSeconds);
				toState.Interpolate(skeleton);

				BinaryInterpolator interpolator(fromState.GetCurrentFrame(), toState.GetCurrentFrame(), m_TransitionResult);
				interpolator.Interpolate(currentTransition.GetWeight());
				if (!m_IsBlending)
				{
					m_IsBlending = true;
				}

				if (currentTransition.IsFinished())
				{
					FinishTransition();
				}
			}
		}
		else
		{
			AnimationState& currentState = GetCurrentState();
			// If the currentState is current but not playing we must start it
			if (!currentState.IsPlaying())
			{
				currentState.StartUp(globalTimeInSeconds);
			}

			currentState.Tick(deltaTimeInSeconds);
			currentState.Interpolate(skeleton);

			// LOG_INFO("LocalTime=%.4f RunningTime=%.4f", currentState.GetNormlizedTime(), currentState.m_RunningTime);
		}
	}

	void AnimationGraph::AddState(const AnimationState& animationState)
	{
		if (!HasState(animationState.GetName()))
		{
			m_States.EmplaceBack(animationState).SetAnimationGraph(this);
		}
	}

	void AnimationGraph::AddState(AnimationState&& animationState)
	{
		if (!HasState(animationState.GetName()))
		{
			m_States.EmplaceBack(animationState).SetAnimationGraph(this);
		}
	}

	void AnimationGraph::RemoveState(const String& name)
	{
		// Remove all transitions using this state
		for (TransitionIterator it = m_Transitions.Begin(); it != m_Transitions.End();)
		{
			if (it->UsesState(name))
			{
				it = m_Transitions.Erase(it);
			}
			else
			{
				it++;
			}
		}

		// Remove state
		for (StateIterator it = m_States.Begin(); it != m_States.End(); it++)
		{
			if (it->GetName() == name)
			{
				m_States.Erase(it);
				return;
			}
		}

		LOG_WARNING("[AnimationGraph::RemoveState] No state with name '%s'", name.c_str());
	}

	void AnimationGraph::AddTransition(const Transition& transition)
	{
		if (!HasTransition(transition.From(), transition.To()))
		{
			m_Transitions.EmplaceBack(transition).SetAnimationGraph(this);
		}
	}

	void AnimationGraph::AddTransition(Transition&& transition)
	{
		if (!HasTransition(transition.From(), transition.To()))
		{
			m_Transitions.EmplaceBack(transition).SetAnimationGraph(this);
		}
	}

	void AnimationGraph::RemoveTransition(const String& fromState, const String& toState)
	{
		for (TransitionIterator it = m_Transitions.Begin(); it != m_Transitions.End(); it++)
		{
			if (it->Equals(fromState, toState))
			{
				m_Transitions.Erase(it);
				return;
			}
		}
	}

	void AnimationGraph::TransitionToState(const String& name)
	{
		if (!HasState(name))
		{
			LOG_WARNING("[AnimationGraph::TransitionToState] No state with name '%s'", name.c_str());
			return;
		}

		AnimationState& currentState = GetCurrentState();
		if (!HasTransition(currentState.GetName(), name))
		{
			LOG_WARNING("[AnimationGraph::TransitionToState] No transition defined from '%s' to '%s'", currentState.GetName().c_str(), name.c_str());
			return;
		}

		// If we already are transitioning we finish it directly
		if (IsTransitioning())
		{
			FinishTransition();
		}

		// Find correct transition
		for (uint32 i = 0; i < m_Transitions.GetSize(); i++)
		{
			if (m_Transitions[i].Equals(currentState.GetName(), name))
			{
				m_CurrentTransition = static_cast<int32>(i);
				break;
			}
		}

		LOG_INFO("Transition from '%s' to '%s'", currentState.GetName().c_str(), name.c_str());
	}

	void AnimationGraph::MakeCurrentState(const String& name)
	{
		for (uint32 i = 0; i < m_States.GetSize(); i++)
		{
			if (m_States[i].GetName() == name)
			{
				GetCurrentState().Stop();

				m_CurrentState = i;
				return;
			}
		}

		LOG_WARNING("[AnimationGraph::MakeCurrentState] No state with name '%s'", name.c_str());
	}

	bool AnimationGraph::HasState(const String& name)
	{
		for (AnimationState& state : m_States)
		{
			if (state.GetName() == name)
			{
				return true;
			}
		}

		return false;
	}

	bool AnimationGraph::HasTransition(const String& fromState, const String& toState)
	{
		for (Transition& transition : m_Transitions)
		{
			if (transition.Equals(fromState, toState))
			{
				return true;
			}
		}

		return false;
	}

	uint32 AnimationGraph::GetStateIndex(const String& name) const
	{
		VALIDATE(m_States.IsEmpty() == false);

		for (uint32 i = 0; i < m_States.GetSize(); i++)
		{
			if (m_States[i].GetName() == name)
			{
				return i;
			}
		}

		return UINT32_MAX;
	}

	AnimationState& AnimationGraph::GetState(uint32 index)
	{
		return m_States[index];
	}

	const AnimationState& AnimationGraph::GetState(uint32 index) const
	{
		return m_States[index];
	}

	AnimationState& AnimationGraph::GetState(const String& name)
	{
		VALIDATE(m_States.IsEmpty() == false);

		for (AnimationState& state : m_States)
		{
			if (state.GetName() == name)
			{
				return state;
			}
		}

		VALIDATE(false);
		return m_States[0];
	}

	const AnimationState& AnimationGraph::GetState(const String& name) const
	{
		VALIDATE(m_States.IsEmpty() == false);

		for (const AnimationState& state : m_States)
		{
			if (state.GetName() == name)
			{
				return state;
			}
		}

		VALIDATE(false);
		return m_States[0];
	}

	AnimationState& AnimationGraph::GetCurrentState()
	{
		return m_States[m_CurrentState];
	}

	const AnimationState& AnimationGraph::GetCurrentState() const
	{
		return m_States[m_CurrentState];
	}

	uint32 AnimationGraph::GetTransitionIndex(const String& fromState, const String& toState)
	{
		VALIDATE(m_Transitions.IsEmpty() == false);

		for (uint32 i = 0; i < m_Transitions.GetSize(); i++)
		{
			if (m_Transitions[i].Equals(fromState, toState))
			{
				return i;
			}
		}

		return UINT32_MAX;
	}

	Transition& AnimationGraph::GetTransition(uint32 index)
	{
		return m_Transitions[index];
	}

	const Transition& AnimationGraph::GetTransition(uint32 index) const
	{
		return m_Transitions[index];
	}

	Transition& AnimationGraph::GetTransition(const String& fromState, const String& toState)
	{
		VALIDATE(m_Transitions.IsEmpty() == false);

		for (Transition& transition : m_Transitions)
		{
			if (transition.Equals(fromState, toState))
			{
				return transition;
			}
		}

		VALIDATE(false);
		return m_Transitions[0];
	}

	const Transition& AnimationGraph::GetTransition(const String& fromState, const String& toState) const
	{
		VALIDATE(m_Transitions.IsEmpty() == false);

		for (const Transition& transition : m_Transitions)
		{
			if (transition.Equals(fromState, toState))
			{
				return transition;
			}
		}

		VALIDATE(false);
		return m_Transitions[0];
	}

	Transition& AnimationGraph::GetCurrentTransition()
	{
		VALIDATE(m_Transitions.IsEmpty() == false);
		VALIDATE(m_CurrentTransition > INVALID_TRANSITION);
		return m_Transitions[m_CurrentTransition];
	}

	const Transition& AnimationGraph::GetCurrentTransition() const
	{
		VALIDATE(m_Transitions.IsEmpty() == false);
		VALIDATE(m_CurrentTransition > INVALID_TRANSITION);
		return m_Transitions[m_CurrentTransition];
	}
	
	const TArray<SQT>& AnimationGraph::GetCurrentFrame() const
	{
		if (m_IsBlending)
		{
			return m_TransitionResult;
		}
		else
		{
			return GetCurrentState().GetCurrentFrame();
		}
	}

	AnimationGraph& AnimationGraph::operator=(AnimationGraph&& other)
	{
		if (this != std::addressof(other))
		{
			m_IsBlending		= other.m_IsBlending;
			m_CurrentTransition	= other.m_CurrentTransition;
			m_CurrentState		= other.m_CurrentState;
			m_States			= other.m_States;
			m_Transitions		= other.m_Transitions;
			m_TransitionResult	= other.m_TransitionResult;

			other.m_IsBlending			= false;
			other.m_CurrentTransition	= -1;
			other.m_CurrentState		= -1;

			SetOwnerGraph();
		}

		return *this;
	}

	AnimationGraph& AnimationGraph::operator=(const AnimationGraph& other)
	{
		if (this != std::addressof(other))
		{
			m_IsBlending		= other.m_IsBlending;
			m_CurrentTransition	= other.m_CurrentTransition;
			m_CurrentState		= other.m_CurrentState;
			m_States			= other.m_States;
			m_Transitions		= other.m_Transitions;
			m_TransitionResult	= other.m_TransitionResult;

			SetOwnerGraph();
		}

		return *this;
	}
	
	void AnimationGraph::FinishTransition()
	{
		GetCurrentTransition().Reset();

		MakeCurrentState(GetCurrentTransition().To());
		m_CurrentTransition = INVALID_TRANSITION;
		m_IsBlending = false;
	}
	
	void AnimationGraph::SetOwnerGraph()
	{
		for (Transition& transition : m_Transitions)
		{
			transition.SetAnimationGraph(this);
		}

		for (AnimationState& state : m_States)
		{
			state.SetAnimationGraph(this);
		}
	}
}