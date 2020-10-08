#pragma once
#include "LambdaEngine.h"
#include "Mesh.h"

#define INFINITE_LOOPS		-1
#define INVALID_TRANSITION	-1

namespace LambdaEngine
{
	/*
	* BinaryInterpolator
	*/

	struct BinaryInterpolator
	{
		inline BinaryInterpolator(const TArray<SQT>& input0, const TArray<SQT>& input1, TArray<SQT>& output)
			: Input0(input0)
			, Input1(input1)
			, Output(output)
		{
		}

		void Interpolate(float32 factor);

		const TArray<SQT>&	Input0;
		const TArray<SQT>&	Input1; 
		TArray<SQT>&		Output;
	};

	/*
	* TransitionState -> Stores the state one transition
	*/

	class Transition
	{
		friend class AnimationGraph;

	public:
		Transition(const String& fromState, const String& toState, float64 beginAt = 0.8);
		~Transition() = default;

		void Tick(float64 currentClipsNormalizedTime);

		bool Equals(const String& fromState, const String& toState) const;

		FORCEINLINE void Reset()
		{
			m_LocalClock	= m_BeginAt;
			m_IsActive		= false;
		}

		FORCEINLINE bool IsFinished() const
		{
			return m_LocalClock >= GetMaxWithEpsilon();
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
			const float64 distance = GetMaxWithEpsilon() - m_BeginAt;
			const float64 traveled = m_LocalClock - m_BeginAt;
			return traveled / distance;
		}

	private:
		constexpr float64 GetMaxWithEpsilon() const
		{
			constexpr float64 EPSILON = 0.025;
			return 1.0 - EPSILON;
		}

	private:
		bool	m_IsActive;
		float64	m_LocalClock;
		float64 m_BeginAt;
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
		AnimationState();
		AnimationState(const String& name, GUID_Lambda animationGUID, bool isLooping = true);
		~AnimationState() = default;

		void Tick(float64 globalTimeInSeconds);
		void Interpolate(const Skeleton& skeleton);

		Animation& GetAnimation() const;

		FORCEINLINE void StartUp(float64 startTime)
		{
			m_StartTime = startTime;
			m_IsPlaying = true;
		}

		FORCEINLINE void Stop()
		{
			m_IsPlaying = false;
		}

		FORCEINLINE bool IsLooping() const
		{
			return m_IsLooping;
		}

		FORCEINLINE bool IsPlaying() const
		{
			return m_IsPlaying;
		}

		FORCEINLINE void SetPlaybackSpeed(float64 speed)
		{
			m_PlaybackSpeed = speed;
		}

		FORCEINLINE float64 GetPlaybackSpeed() const
		{
			return m_PlaybackSpeed;
		}

		FORCEINLINE float64 GetNormlizedTime() const
		{
			return m_NormalizedTime;
		}

		FORCEINLINE float64 GetDurationInSeconds() const
		{
			return m_DurationInSeconds;
		}

		FORCEINLINE float64 GetLocalTimeInSeconds() const
		{
			return m_LocalTimeInSeconds;
		}

		FORCEINLINE const String& GetName() const
		{
			return m_Name;
		}

		FORCEINLINE GUID_Lambda GetAnimationID() const
		{
			return m_AnimationGUID;
		}

		FORCEINLINE const TArray<SQT>& GetCurrentFrame() const
		{
			return m_CurrentFrame;
		}

	private:
		glm::vec3 SamplePosition(Animation::Channel& channel, float64 time);
		glm::vec3 SampleScale(Animation::Channel& channel, float64 time);
		glm::quat SampleRotation(Animation::Channel& channel, float64 time);

	private:
		bool m_IsLooping;
		bool m_IsPlaying;

		uint64	m_NumLoops;
		float64	m_StartTime;
		float64	m_PlaybackSpeed;
		float64	m_NormalizedTime;
		float64	m_LocalTimeInSeconds;
		float64	m_DurationInSeconds;
		
		String		m_Name;
		GUID_Lambda	m_AnimationGUID;
		TArray<SQT> m_CurrentFrame;
	};

	/*
	* AnimationGraph -> Contains multiple AnimationStates, and control the transitions between these states
	*/

	class AnimationGraph
	{
		using StateIterator			= TArray<AnimationState>::Iterator;
		using TransitionIterator	= TArray<Transition>::Iterator;

	public:
		AnimationGraph();
		explicit AnimationGraph(const AnimationState& animationState);
		explicit AnimationGraph(AnimationState&& animationState);
		~AnimationGraph() = default;

		void Tick(float64 deltaTimeSeconds, float64 globalTimeInSeconds, const Skeleton& skeleton);

		// Adds a new state to the graph if there currently are no state with the same name
		void AddState(const AnimationState& animationState);
		void AddState(AnimationState&& animationState);

		// Removes the state with the specifed name
		void RemoveState(const String& name);

		// Adds a new transition to the graph, if it is valid (If there exists two states specified in the transition)
		void AddTransition(const Transition& transition);
		void AddTransition(Transition&& transition);

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

		AnimationState& GetState(const String& name);
		const AnimationState& GetState(const String& name) const;

		AnimationState& GetCurrentState();
		const AnimationState& GetCurrentState() const;

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
		bool m_IsBlending;

		int32	m_CurrentTransition;
		uint32	m_CurrentState;

		TArray<AnimationState>	m_States;
		TArray<Transition>		m_Transitions;
		TArray<SQT>				m_TransitionResult;
	};
}