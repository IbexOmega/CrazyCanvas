#pragma once
#include "LambdaEngine.h"
#include "Mesh.h"

#define INFINITE_LOOPS		-1
#define INVALID_TRANSITION	-1

namespace LambdaEngine
{
	class AnimationGraph;
	class AnimationState;
	
	/*
	* BinaryInterpolator
	*/

	struct BinaryInterpolator
	{
		inline BinaryInterpolator(const TArray<SQT>& in0, const TArray<SQT>& in1, TArray<SQT>& out)
			: In0(in0)
			, In1(in1)
			, Out(out)
		{
		}

		void Interpolate(float32 factor);

		const TArray<SQT>&	In0;
		const TArray<SQT>&	In1; 
		TArray<SQT>&		Out;
	};

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

		FORCEINLINE void Reset()
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
	* BlendInfo -> Stores blending animation (Animation state can blend two animations)
	*/

	struct BlendInfo
	{
		inline BlendInfo()
			: AnimationGUID(GUID_NONE)
			, Weight(0.0f)
			, PlaybackSpeed(1.0f)
		{
		}

		inline BlendInfo(GUID_Lambda animationGUID, float32 weight, float32 playbackSpeed)
			: AnimationGUID(animationGUID)
			, Weight(weight)
			, PlaybackSpeed(playbackSpeed)
		{
		}

		GUID_Lambda	AnimationGUID;
		float32		Weight;
		float32		PlaybackSpeed;
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

		void Tick(const float64 deltaTime);
		void Interpolate(const Skeleton& skeleton);

		FORCEINLINE void SetBlendInfo(const BlendInfo& blendInfo)
		{
			m_BlendInfo = blendInfo;
		}

		Animation& GetAnimation() const;
		Animation& GetBlendAnimation() const;

		FORCEINLINE void StartUp(float64 startTime, float64 startAt = 0.0)
		{
			startAt = glm::clamp(startAt, 0.0, 1.0);

			m_StartTime		= startTime;
			m_RunningTime	= 0.0;
			m_IsPlaying		= true;
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

		FORCEINLINE void SetIsLooping(bool isLooping)
		{
			m_IsLooping = isLooping;
		}

		FORCEINLINE void SetNumLoops(uint32 numLoops)
		{
			m_NumLoops = numLoops;
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
			return m_CurrentFrame0;
		}

	private:
		glm::vec3 SamplePosition(Animation::Channel& channel, float64 time);
		glm::vec3 SampleScale(Animation::Channel& channel, float64 time);
		glm::quat SampleRotation(Animation::Channel& channel, float64 time);

		void InternalInterpolate(Animation& animation, const Skeleton& skeleton, TArray<SQT>& currentFrame, float64 timestamp);
		float64 InternalCalculateLocalTime(float64 playbackSpeed, float64 durationInSeconds);

		FORCEINLINE void SetAnimationGraph(AnimationGraph* pGraph)
		{
			VALIDATE(pGraph != nullptr);
			m_pOwnerGraph = pGraph;
		}

	private:
		AnimationGraph* m_pOwnerGraph;
		BlendInfo m_BlendInfo;

		bool m_IsLooping;
		bool m_IsPlaying;

		uint64	m_NumLoops;
		float64	m_StartTime;
		float64	m_RunningTime;
		float64	m_PlaybackSpeed;
		float64	m_NormalizedTime;
		float64	m_LocalTimeInSeconds;
		float64	m_DurationInSeconds;
		
		String		m_Name;
		GUID_Lambda	m_AnimationGUID;
		TArray<SQT> m_CurrentFrame0; // Used for main animation
		TArray<SQT> m_CurrentFrame1; // Used for blend animation
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
		AnimationGraph(AnimationGraph&& other);
		AnimationGraph(const AnimationGraph& other);
		~AnimationGraph() = default;

		void Tick(float64 deltaTimeInSeconds, float64 globalTimeInSeconds, const Skeleton& skeleton);

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

		AnimationGraph& operator=(AnimationGraph&& other);
		AnimationGraph& operator=(const AnimationGraph& other);

		FORCEINLINE bool IsTransitioning() const
		{
			return (m_CurrentTransition > INVALID_TRANSITION);
		}

	private:
		void FinishTransition();
		void SetOwnerGraph();

	private:
		bool m_IsBlending;

		int32	m_CurrentTransition;
		uint32	m_CurrentState;

		TArray<AnimationState>	m_States;
		TArray<SQT>				m_TransitionResult;
		TArray<Transition>		m_Transitions;
	};
}