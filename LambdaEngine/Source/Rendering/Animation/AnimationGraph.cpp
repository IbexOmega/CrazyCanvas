#include "Rendering/Animation/AnimationGraph.h"
#include <sstream>

namespace LambdaEngine
{
	/*
	* Transition
	*/

	Transition::Transition(const String& fromState, const String& toState, float64 duration)
		: m_pOwnerGraph(nullptr)
		, m_pFrom(nullptr)
		, m_pTo(nullptr)
		, m_IsActive(false)
		, m_FromState(fromState)
		, m_Duration(duration)
		, m_LocalClock(0.0f)
		, m_ToState(toState)
	{
		OnFinished();
	}

	void Transition::Tick(const Skeleton& skeleton, const float64 deltaTimeInSeconds)
	{
		VALIDATE(m_pFrom	!= nullptr);
		VALIDATE(m_pTo		!= nullptr);

		if (fabs(m_Duration) > 0.0)
		{
			// Tick the states
			m_pFrom->Tick(skeleton, deltaTimeInSeconds);
			m_pTo->Tick(skeleton, deltaTimeInSeconds);

			// Move clock
			m_LocalClock += deltaTimeInSeconds;
			const float64 weight = (m_LocalClock / (m_Duration));

#if 0
			LOG_INFO("From='%s', To='%s' Weight=%.4f", m_pFrom->GetName().c_str(), m_pTo->GetName().c_str(), weight);
#endif
			const TArray<SQT>& in0 = m_pFrom->GetCurrentFrame();
			const TArray<SQT>& in1 = m_pTo->GetCurrentFrame();
			BinaryInterpolator interpolator(in0, in1, m_CurrentFrame);
			interpolator.Interpolate(float32(weight));
		}
		else
		{
			m_pTo->Tick(skeleton, deltaTimeInSeconds);
			m_CurrentFrame = m_pTo->GetCurrentFrame();
		}
	}

	bool Transition::Equals(const String& fromState, const String& toState) const
	{
		return m_FromState == fromState && m_ToState == toState;
	}

	bool Transition::Equals(AnimationState* pFromState, AnimationState* pToState) const
	{
		VALIDATE(pFromState != nullptr);
		VALIDATE(pFromState->GetOwner() == m_pOwnerGraph);
		VALIDATE(pToState != nullptr);
		VALIDATE(pToState->GetOwner() == m_pOwnerGraph);
		return m_pFrom == pFromState || m_pTo == pToState;
	}

	bool Transition::UsesState(const String& state) const
	{
		return m_FromState == state || m_ToState == state;
	}

	bool Transition::UsesState(AnimationState* pState) const
	{
		VALIDATE(pState != nullptr);
		VALIDATE(pState->GetOwner() == m_pOwnerGraph);
		return m_pFrom == pState || m_pTo == pState;
	}

	void Transition::SetAnimationGraph(AnimationGraph* pGraph)
	{
		VALIDATE(pGraph != nullptr);

		m_pOwnerGraph	= pGraph;
		
		m_pFrom = m_pOwnerGraph->GetState(m_FromState);
		VALIDATE(m_pFrom != nullptr);
		VALIDATE(m_pFrom->GetOwner() == m_pOwnerGraph);
		
		m_pTo = m_pOwnerGraph->GetState(m_ToState);
		VALIDATE(m_pTo != nullptr);
		VALIDATE(m_pTo->GetOwner() == m_pOwnerGraph);
	}
	
	const TArray<SQT>& Transition::GetCurrentFrame() const
	{
		return m_CurrentFrame;
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
#if 0
		LOG_INFO("Tick '%s'", m_Name.c_str());
#endif
		m_pFinalNode->Tick(skeleton, deltaTime);
	}

	void AnimationState::Reset()
	{
#if 0
		LOG_INFO("Reset %s", m_Name.c_str());
#endif
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
		, m_pCurrentTransition(nullptr)
		, m_pCurrentState(nullptr)
		, m_States()
		, m_Transitions()
	{
	}

	AnimationGraph::AnimationGraph(AnimationState* pAnimationState)
		: m_IsBlending(false)
		, m_pCurrentTransition(nullptr)
		, m_pCurrentState(nullptr)
		, m_States()
		, m_Transitions()
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

	void AnimationGraph::Tick(const Skeleton& skeleton, float64 deltaTimeInSeconds)
	{
		// Handle transition
		if (IsTransitioning())
		{
			Transition* pCurrentTransition = GetCurrentTransition();
			if (pCurrentTransition->IsFinished())
			{
				FinishTransition();
			}
			else
			{
				pCurrentTransition->Tick(skeleton, deltaTimeInSeconds);
				return;
			}
		}

		AnimationState* pCurrentState = GetCurrentState();
		pCurrentState->Tick(skeleton, deltaTimeInSeconds);
	}

	bool AnimationGraph::AddState(AnimationState* pAnimationState)
	{
		if (!HasState(pAnimationState->GetName()))
		{
			AnimationState* pNewState = m_States.EmplaceBack(pAnimationState);
			pNewState->SetAnimationGraph(this);
			
			// If this is the first state we add, set it to current
			if (m_pCurrentState == nullptr)
			{
				m_pCurrentState = pNewState;
			}

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
		if (!HasTransition(pTransition->GetFromStateName(), pTransition->GetToStateName()))
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

		LOG_WARNING("[AnimationGraph::RemoveTransition] No transition from '%s' to '%s' exists", fromState.c_str(), toState.c_str());
	}

	void AnimationGraph::TransitionToState(const String& name)
	{
		if (!HasState(name))
		{
			LOG_WARNING("[AnimationGraph::TransitionToState] No state with name '%s'", name.c_str());
			return;
		}

		AnimationState* pCurrentState = GetCurrentState();
		if (!HasTransition(pCurrentState->GetName(), name))
		{
			LOG_WARNING("[AnimationGraph::TransitionToState] No transition defined from '%s' to '%s'", pCurrentState->GetName().c_str(), name.c_str());
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
			if (m_Transitions[i]->Equals(pCurrentState->GetName(), name))
			{
				m_pCurrentTransition = m_Transitions[i];
				break;
			}
		}
	}

	void AnimationGraph::MakeCurrentState(const String& name)
	{
		// Do nothing if we already are in correct state
		if (GetCurrentState()->GetName() == name)
		{
			return;
		}

		// Otherwise find, reset current, and set new state
		for (uint32 i = 0; i < m_States.GetSize(); i++)
		{
			if (m_States[i]->GetName() == name)
			{
				GetCurrentState()->Reset();
				m_pCurrentState = m_States[i];
				return;
			}
		}

		LOG_WARNING("[AnimationGraph::MakeCurrentState] No state with name '%s'", name.c_str());
	}

	void AnimationGraph::MakeCurrentState(AnimationState* pState)
	{
		if (pState->GetOwner() != this)
		{
			LOG_ERROR("[AnimationGraph::MakeCurrentState] This graph is not owner of specified state");
			return;
		}

		if (m_pCurrentState != pState)
		{
			m_pCurrentState->Reset();
			m_pCurrentState = pState;
		}
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

	AnimationState* AnimationGraph::GetState(const String& name) const
	{
		VALIDATE(!m_States.IsEmpty());

		for (AnimationState* pState : m_States)
		{
			if (pState->GetName() == name)
			{
				return pState;
			}
		}

		return nullptr;
	}

	Transition* AnimationGraph::GetTransition(const String& fromState, const String& toState) const
	{
		VALIDATE(m_Transitions.IsEmpty() == false);

		for (Transition* pTransition : m_Transitions)
		{
			if (pTransition->Equals(fromState, toState))
			{
				return pTransition;
			}
		}

		return nullptr;
	}

	Transition* AnimationGraph::GetTransition(AnimationState* pFromState, AnimationState* pToState) const
	{
		VALIDATE(m_Transitions.IsEmpty() == false);

		for (Transition* pTransition : m_Transitions)
		{
			if (pTransition->Equals(pFromState, pToState))
			{
				return pTransition;
			}
		}

		return nullptr;
	}
	
	const TArray<SQT>& AnimationGraph::GetCurrentFrame() const
	{
		if (IsTransitioning())
		{
			return GetCurrentTransition()->GetCurrentFrame();
		}
		else
		{
			return GetCurrentState()->GetCurrentFrame();
		}
	}
		
	void AnimationGraph::FinishTransition()
	{
		GetCurrentTransition()->OnFinished();

		MakeCurrentState(GetCurrentTransition()->GetToState());
		m_pCurrentTransition	= nullptr;
		m_IsBlending			= false;
	}
}