#include "Rendering/Animation/AnimationGraph.h"

namespace LambdaEngine
{
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
		OnFinished();
	}

	void Transition::Tick(const float64 delta)
	{
		VALIDATE(m_From != UINT32_MAX);

		// Only transition if the clip has reached correct timing
		AnimationState& fromState = m_pOwnerGraph->GetState(m_From);
		const float64 normalizedTime = fromState.GetNormalizedTime();

		//LOG_INFO("normalizedTime=%.4f, m_FromBeginAt=%.4f", normalizedTime, m_FromBeginAt);

		// This makes sure that we loop around one time if we start a transition after the sync-point
		if (normalizedTime >= m_FromBeginAt)
		{
			m_LocalClock = normalizedTime;
		}

		// Calculate deltatime, used for determine if we should finish transition
		if (m_LastTime > 0.0)
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
		, m_pFinalNode(nullptr)
		, m_NodeAllocator()
		, m_Nodes()
		, m_Name()
	{
		m_pFinalNode = CreateOutputNode(nullptr);
	}

	AnimationState::AnimationState(const String& name)
		: m_pOwnerGraph(nullptr)
		, m_pFinalNode(nullptr)
		, m_NodeAllocator()
		, m_Nodes()
		, m_Name(name)
	{
		m_pFinalNode = CreateOutputNode(nullptr);
	}

	AnimationState::AnimationState(const String& name, GUID_Lambda animationGUID, float64 playbackSpeed, bool isLooping)
		: m_pOwnerGraph(nullptr)
		, m_pFinalNode(nullptr)
		, m_NodeAllocator()
		, m_Nodes()
		, m_Name(name)
	{
		ClipNode* pClip = CreateClipNode(animationGUID, playbackSpeed, isLooping);
		m_pFinalNode = CreateOutputNode(pClip);
	}

	AnimationState::~AnimationState()
	{
		for (AnimationNode* pNode : m_Nodes)
		{
			pNode->~AnimationNode();
		}
	}

	void AnimationState::Tick(const Skeleton& skeleton, const float64 deltaTime)
	{
		m_pFinalNode->Tick(skeleton, deltaTime);
	}

	void AnimationState::Reset()
	{
		m_pFinalNode->Reset();
	}

	ClipNode* AnimationState::CreateClipNode(GUID_Lambda animationGUID, float64 playbackSpeed, bool isLooping)
	{
		void* pMemory = m_NodeAllocator.Allocate<ClipNode>();
		ClipNode* pClipNode = new(pMemory) ClipNode(this, animationGUID, playbackSpeed, isLooping);
		m_Nodes.EmplaceBack(pClipNode);
		return pClipNode;
	}

	BlendNode* AnimationState::CreateBlendNode(AnimationNode* pIn0, AnimationNode* pIn1, const BlendInfo& blendInfo)
	{
		void* pMemory = m_NodeAllocator.Allocate<BlendNode>();
		BlendNode* pBlendNote = new(pMemory) BlendNode(this, pIn0, pIn1, blendInfo);
		m_Nodes.EmplaceBack(pBlendNote);
		return pBlendNote;
	}

	OutputNode* AnimationState::CreateOutputNode(AnimationNode* pInput)
	{
		void* pMemory = m_NodeAllocator.Allocate<OutputNode>();
		OutputNode* pOutput = new(pMemory) OutputNode(this, pInput);
		m_Nodes.EmplaceBack(pOutput);
		return pOutput;
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

	AnimationGraph::AnimationGraph(AnimationState* pAnimationState)
		: m_IsBlending(false)
		, m_CurrentTransition(INVALID_TRANSITION)
		, m_CurrentState(0)
		, m_States()
		, m_Transitions()
		, m_TransitionResult()
	{
		AddState(pAnimationState);
	}

	AnimationGraph::~AnimationGraph()
	{
		for (AnimationState* pState : m_States)
		{
			SAFEDELETE(pState);
		}

		for (Transition* pTransition : m_Transitions)
		{
			SAFEDELETE(pTransition);
		}
	}

	void AnimationGraph::Tick(float64 deltaTimeInSeconds, float64 globalTimeInSeconds, const Skeleton& skeleton)
	{
		AnimationState& currentState = GetCurrentState();
		currentState.Tick(skeleton, deltaTimeInSeconds);

		// Handle transition
		if (IsTransitioning())
		{
			Transition& currentTransition = GetCurrentTransition();
			currentTransition.Tick(deltaTimeInSeconds);

			LOG_INFO("Weight=%.4f, LocalTime=%.4f", currentTransition.GetWeight(), currentState.GetNormalizedTime());

			VALIDATE(HasState(currentTransition.To()));

			AnimationState& toState = GetState(currentTransition.To());
			if (currentTransition.GetWeight() > 0.0f)
			{
				toState.Tick(skeleton, deltaTimeInSeconds);

				BinaryInterpolator interpolator(currentState.GetCurrentFrame(), toState.GetCurrentFrame(), m_TransitionResult);
				interpolator.Interpolate(currentTransition.GetWeight());
				if (!m_IsBlending)
				{
					m_IsBlending = true;
				}

				if (currentTransition.IsFinished())
				{
					LOG_INFO("FINISHED TRANSITION", currentTransition.GetWeight(), currentState.GetNormalizedTime());
					
					FinishTransition();
				}
			}
		}
	}

	bool AnimationGraph::AddState(AnimationState* pAnimationState)
	{
		if (!HasState(pAnimationState->GetName()))
		{
			m_States.EmplaceBack(pAnimationState)->SetAnimationGraph(this);
			return true;
		}
		else
		{
			return false;
		}
	}

	void AnimationGraph::RemoveState(const String& name)
	{
		// Remove all transitions using this state
		for (TransitionIterator it = m_Transitions.Begin(); it != m_Transitions.End();)
		{
			if ((*it)->UsesState(name))
			{
				SAFEDELETE((*it));
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
			if ((*it)->GetName() == name)
			{
				SAFEDELETE((*it));
				m_States.Erase(it);

				return;
			}
		}

		LOG_WARNING("[AnimationGraph::RemoveState] No state with name '%s'", name.c_str());
	}

	bool AnimationGraph::AddTransition(Transition* pTransition)
	{
		if (!HasTransition(pTransition->From(), pTransition->To()))
		{
			m_Transitions.EmplaceBack(pTransition)->SetAnimationGraph(this);
			return true;
		}
		else
		{
			return false;
		}
	}

	void AnimationGraph::RemoveTransition(const String& fromState, const String& toState)
	{
		for (TransitionIterator it = m_Transitions.Begin(); it != m_Transitions.End(); it++)
		{
			if ((*it)->Equals(fromState, toState))
			{
				SAFEDELETE((*it));
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
			if (m_Transitions[i]->Equals(currentState.GetName(), name))
			{
				m_CurrentTransition = static_cast<int32>(i);
				break;
			}
		}

		LOG_INFO("Transition from '%s' to '%s'", currentState.GetName().c_str(), name.c_str());
	}

	void AnimationGraph::MakeCurrentState(const String& name)
	{
		// Do nothing if we already are in correct state
		if (GetCurrentState().GetName() == name)
		{
			return;
		}

		// Otherwise find, reset current, and set new state
		for (uint32 i = 0; i < m_States.GetSize(); i++)
		{
			if (m_States[i]->GetName() == name)
			{
				GetCurrentState().Reset();

				m_CurrentState = i;
				return;
			}
		}

		LOG_WARNING("[AnimationGraph::MakeCurrentState] No state with name '%s'", name.c_str());
	}

	bool AnimationGraph::HasState(const String& name)
	{
		for (AnimationState* pState : m_States)
		{
			if (pState->GetName() == name)
			{
				return true;
			}
		}

		return false;
	}

	bool AnimationGraph::HasTransition(const String& fromState, const String& toState)
	{
		for (Transition* pTransition : m_Transitions)
		{
			if (pTransition->Equals(fromState, toState))
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
			if (m_States[i]->GetName() == name)
			{
				return i;
			}
		}

		return UINT32_MAX;
	}

	AnimationState& AnimationGraph::GetState(uint32 index)
	{
		return *m_States[index];
	}

	const AnimationState& AnimationGraph::GetState(uint32 index) const
	{
		return *m_States[index];
	}

	AnimationState& AnimationGraph::GetState(const String& name)
	{
		VALIDATE(m_States.IsEmpty() == false);

		for (AnimationState* pState : m_States)
		{
			if (pState->GetName() == name)
			{
				return *pState;
			}
		}

		VALIDATE(false);
		return *m_States[0];
	}

	const AnimationState& AnimationGraph::GetState(const String& name) const
	{
		VALIDATE(m_States.IsEmpty() == false);

		for (AnimationState* pState : m_States)
		{
			if (pState->GetName() == name)
			{
				return *pState;
			}
		}

		VALIDATE(false);
		return *m_States[0];
	}

	AnimationState& AnimationGraph::GetCurrentState()
	{
		return *m_States[m_CurrentState];
	}

	const AnimationState& AnimationGraph::GetCurrentState() const
	{
		return *m_States[m_CurrentState];
	}

	uint32 AnimationGraph::GetTransitionIndex(const String& fromState, const String& toState)
	{
		VALIDATE(m_Transitions.IsEmpty() == false);

		for (uint32 i = 0; i < m_Transitions.GetSize(); i++)
		{
			if (m_Transitions[i]->Equals(fromState, toState))
			{
				return i;
			}
		}

		return UINT32_MAX;
	}

	Transition& AnimationGraph::GetTransition(uint32 index)
	{
		return *m_Transitions[index];
	}

	const Transition& AnimationGraph::GetTransition(uint32 index) const
	{
		return *m_Transitions[index];
	}

	Transition& AnimationGraph::GetTransition(const String& fromState, const String& toState)
	{
		VALIDATE(m_Transitions.IsEmpty() == false);

		for (Transition* pTransition : m_Transitions)
		{
			if (pTransition->Equals(fromState, toState))
			{
				return *pTransition;
			}
		}

		VALIDATE(false);
		return *m_Transitions[0];
	}

	const Transition& AnimationGraph::GetTransition(const String& fromState, const String& toState) const
	{
		VALIDATE(m_Transitions.IsEmpty() == false);

		for (const Transition* pTransition : m_Transitions)
		{
			if (pTransition->Equals(fromState, toState))
			{
				return *pTransition;
			}
		}

		VALIDATE(false);
		return *m_Transitions[0];
	}

	Transition& AnimationGraph::GetCurrentTransition()
	{
		VALIDATE(m_Transitions.IsEmpty() == false);
		VALIDATE(m_CurrentTransition > INVALID_TRANSITION);
		return *m_Transitions[m_CurrentTransition];
	}

	const Transition& AnimationGraph::GetCurrentTransition() const
	{
		VALIDATE(m_Transitions.IsEmpty() == false);
		VALIDATE(m_CurrentTransition > INVALID_TRANSITION);
		return *m_Transitions[m_CurrentTransition];
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
		
	void AnimationGraph::FinishTransition()
	{
		GetCurrentTransition().OnFinished();

		MakeCurrentState(GetCurrentTransition().To());
		m_CurrentTransition = INVALID_TRANSITION;
		m_IsBlending = false;
	}
}