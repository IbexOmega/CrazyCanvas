#pragma once
#include "LambdaEngine.h"
#include "AnimationNode.h"

#include "Memory/API/StackAllocator.h"

namespace LambdaEngine
{
	class AnimationGraph;
	class AnimationState;

	/*
	* Transition -> Stores the state of one transition
	*/

	class Transition
	{
		friend class AnimationGraph;

	public:
		Transition(const String& fromState, const String& toState, float64 duration = 0.0);
		~Transition() = default;

		void Tick(const Skeleton& skeleton, const float64 deltaTimeInSeconds);

		bool Equals(const String& fromState, const String& toState) const;
		bool Equals(AnimationState* pFromState, AnimationState* pToState) const;
		bool UsesState(const String& state) const;
		bool UsesState(AnimationState* pState) const;

		const TArray<SQT>& GetCurrentFrame() const;

		FORCEINLINE void OnFinished()
		{
			m_LocalClock	= 0.0;
			m_IsActive		= false;
		}

		FORCEINLINE bool IsFinished() const
		{
			return m_LocalClock >= m_Duration;
		}

		FORCEINLINE const String& GetFromStateName() const
		{
			return m_FromState;
		}

		FORCEINLINE const String& GetToStateName() const
		{
			return m_ToState;
		}

		FORCEINLINE AnimationState* GetFromState() const
		{
			return m_pFrom;
		}

		FORCEINLINE AnimationState* GetToState() const
		{
			return m_pTo;
		}

	private:
		void SetAnimationGraph(AnimationGraph* pGraph);

	private:
		AnimationGraph* m_pOwnerGraph;
		AnimationState* m_pFrom;
		AnimationState* m_pTo;

		bool	m_IsActive;
		float64	m_Duration;
		float64	m_LocalClock;
		String	m_FromState;
		String	m_ToState;

		TArray<SQT> m_CurrentFrame;
	};

	/*
	* AnimationState -> Stores the state of one animation
	*/

	class AnimationState
	{
		friend class AnimationGraph;

	public:
		DECL_REMOVE_COPY(AnimationState);
		DECL_REMOVE_MOVE(AnimationState);

		AnimationState();
		AnimationState(const String& name);
		AnimationState(const String& name, GUID_Lambda animationGUID, float64 playbackSpeed = 1.0f, bool isLooping = true);
		~AnimationState();

		void Tick(const Skeleton& skeleton, const float64 deltaTimeInSeconds);
		void Reset();

		ClipNode*	CreateClipNode(GUID_Lambda animationGUID, float64 playbackSpeed = 1.0f, bool isLooping = true);
		BlendNode*	CreateBlendNode(AnimationNode* pIn0, AnimationNode* pIn1, const BlendInfo& blendInfo);

		FORCEINLINE void SetOutputNode(AnimationNode* pOutput)
		{
			m_pFinalNode->SetInputNode(pOutput);
		}

		FORCEINLINE float64 GetNormalizedTime() const
		{
			const float64 duration	= m_pFinalNode->GetDurationInSeconds();
			const float64 local		= m_pFinalNode->GetLocalTimeInSeconds();
			return local / duration;
		}

		FORCEINLINE const TArray<SQT>& GetCurrentFrame() const
		{
			return m_pFinalNode->GetResult();
		}

		FORCEINLINE const String& GetName() const
		{
			return m_Name;
		}

		FORCEINLINE AnimationGraph* GetOwner() const
		{
			return m_pOwnerGraph;
		}

	private:
		OutputNode* CreateOutputNode(AnimationNode* pInput);

		FORCEINLINE void SetAnimationGraph(AnimationGraph* pGraph)
		{
			VALIDATE(pGraph != nullptr);
			m_pOwnerGraph = pGraph;
		}

	private:
		AnimationGraph*	m_pOwnerGraph;
		OutputNode*		m_pFinalNode;
		StackAllocator	m_NodeAllocator;
		TArray<AnimationNode*> m_Nodes;
		String m_Name;
	};

	/*
	* AnimationGraph:
	* Contains multiple AnimationStates, and control the transitions between these states.
	* Unique for each animationcomponent, and should therefore; not be copied.
	*/

	class AnimationGraph
	{
		using StateIterator			= TArray<AnimationState*>::Iterator;
		using TransitionIterator	= TArray<Transition*>::Iterator;

	public:
		DECL_REMOVE_COPY(AnimationGraph);
		DECL_REMOVE_MOVE(AnimationGraph);

		AnimationGraph();
		AnimationGraph(AnimationState* pAnimationState);
		~AnimationGraph();

		void Tick(const Skeleton& skeleton, float64 deltaTimeInSeconds);

		// Adds a new state to the graph if there currently are no state with the same name
		// If AddState returns true the AnimationGraph has ownership if false YOU have to call delete
		bool AddState(AnimationState* pAnimationState);

		// Removes the state with the specifed name
		void RemoveState(const String& name);

		// Adds a new transition to the graph, if it is valid (If there exists two states specified in the transition)
		// If AddTransition returns true the AnimationGraph has ownership if false YOU have to call delete
		bool AddTransition(Transition* pTransition);

		// Removes the state with the specifed name
		void RemoveTransition(const String& fromState, const String& toState);

		// Transitions into another state if there exists a transition between the states
		void TransitionToState(const String& name);

		// Forces current state to be the specified state (This does not require a transition)
		void MakeCurrentState(const String& name);
		void MakeCurrentState(AnimationState* pState);

		// Returns true if the graph contains a state with the specified name
		bool HasState(const String& name);

		// Returns true if the graph contains a transition with the specified from and to
		bool HasTransition(const String& fromState, const String& toState);

		const TArray<SQT>& GetCurrentFrame() const;

		// Returns nullptr if the state does not exist
		AnimationState* GetState(const String& name) const;

		// Returns nullptr if the transition does not exist
		Transition* GetTransition(AnimationState* pFromState, AnimationState* pToState) const;
		Transition* GetTransition(const String& fromState, const String& toState) const;

		FORCEINLINE Transition* GetCurrentTransition() const
		{
			return m_pCurrentTransition;
		}
		
		FORCEINLINE AnimationState* GetCurrentState() const
		{
			return m_pCurrentState;
		}

		FORCEINLINE bool IsTransitioning() const
		{
			return m_pCurrentTransition != nullptr;
		}

	private:
		void FinishTransition();

	private:
		bool m_IsBlending;

		Transition*		m_pCurrentTransition;
		AnimationState* m_pCurrentState;

		TArray<AnimationState*>	m_States;
		TArray<Transition*>		m_Transitions;
	};
}