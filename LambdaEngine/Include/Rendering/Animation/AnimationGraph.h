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
		Transition(const String& fromState, const String& toState, float64 fromBeginAt = 0.8, float64 toBeginAt = 0.0);
		~Transition() = default;

		void Tick(const float64 delta);

		bool Equals(const String& fromState, const String& toState) const;
		bool UsesState(const String& state) const;

		FORCEINLINE void OnFinished()
		{
			m_LocalClock	= m_FromBeginAt;
			m_IsActive		= false;
			m_LastTime		= 0.0;
			m_DeltaTime		= 0.0;
		}

		FORCEINLINE bool IsFinished() const
		{
			// Use deltatime to predict if we are finished (This prevents the animation to replay due to large DT)
			return (m_LocalClock + m_DeltaTime) >= 1.0;
		}

		FORCEINLINE const String& From() const
		{
			return m_FromState;
		}

		FORCEINLINE const String& To() const
		{
			return m_ToState;
		}

		FORCEINLINE float64 GetWeight() const
		{
			const float64 distance = 1.0 - m_FromBeginAt;
			const float64 traveled = m_LocalClock - m_FromBeginAt;
			return traveled / distance;
		}

		FORCEINLINE float64 GetFromBeginAt() const
		{
			return m_FromBeginAt;
		}

		FORCEINLINE float64 GetToBeginAt() const
		{
			return m_ToBeginAt;
		}

	private:
		void SetAnimationGraph(AnimationGraph* pGraph);

	private:
		AnimationGraph* m_pOwnerGraph;
		uint32 m_From;
		uint32 m_To;

		bool	m_IsActive;
		float64	m_DeltaTime;
		float64	m_LastTime;
		float64	m_LocalClock;
		float64 m_FromBeginAt;
		float64 m_ToBeginAt;
		String	m_FromState;
		String	m_ToState;
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

		FORCEINLINE void OnFinished() const
		{
			if (m_OnFinished)
			{
				VALIDATE(m_pOwnerGraph != nullptr);
				m_OnFinished(*m_pOwnerGraph);
			}
		}

		FORCEINLINE void SetOnFinished(const std::function<void(AnimationGraph&)>& onFinished)
		{
			m_OnFinished = onFinished;
		}

		FORCEINLINE const String& GetName() const
		{
			return m_Name;
		}

		FORCEINLINE const TArray<SQT>& GetCurrentFrame() const
		{
			return m_pFinalNode->GetResult();
		}

		FORCEINLINE void SetOutputNode(AnimationNode* pOutput)
		{
			m_pFinalNode->SetInputNode(pOutput);
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

		std::function<void(AnimationGraph&)> m_OnFinished;
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

		void Tick(float64 deltaTimeInSeconds, float64 globalTimeInSeconds, const Skeleton& skeleton);

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

		// Returns true if the graph contains a state with the specified name
		bool HasState(const String& name);

		// Returns true if the graph contains a transition with the specified from and to
		bool HasTransition(const String& fromState, const String& toState);

		// Returns UINT32_MAX if not found
		uint32 GetStateIndex(const String& name) const;

		AnimationState& GetState(uint32 index);
		const AnimationState& GetState(uint32 index) const;
		AnimationState& GetState(const String& name);
		const AnimationState& GetState(const String& name) const;

		AnimationState& GetCurrentState();
		const AnimationState& GetCurrentState() const;

		// Returns UINT32_MAX if not found
		uint32 GetTransitionIndex(const String& fromState, const String& toState);

		Transition& GetTransition(uint32 index);
		const Transition& GetTransition(uint32 index) const;
		Transition& GetTransition(const String& fromState, const String& toState);
		const Transition& GetTransition(const String& fromState, const String& toState) const;

		Transition& GetCurrentTransition();
		const Transition& GetCurrentTransition() const;

		const TArray<SQT>& GetCurrentFrame() const;

		FORCEINLINE bool IsTransitioning() const
		{
			return (m_CurrentTransition > INVALID_TRANSITION);
		}

	private:
		void FinishTransition();

	private:
		bool m_IsBlending;

		int32	m_CurrentTransition;
		uint32	m_CurrentState;

		TArray<AnimationState*>	m_States;
		TArray<Transition*>		m_Transitions;
		TArray<SQT>				m_TransitionResult;
	};
}