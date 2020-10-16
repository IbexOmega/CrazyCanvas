#pragma once
#include "LambdaEngine.h"

#include "Resources/Mesh.h"

#define INFINITE_LOOPS		-1
#define INVALID_TRANSITION	-1

namespace LambdaEngine
{
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

		const TArray<SQT>& In0;
		const TArray<SQT>& In1;
		TArray<SQT>& Out;
	};

	/*
	* AnimationNode
	*/

	class AnimationState;

	class AnimationNode
	{
	public:
		inline AnimationNode(AnimationState* pParent)
			: m_pParent(pParent)
		{
			VALIDATE(m_pParent != nullptr);
		}

		virtual ~AnimationNode() = default;

		virtual void Tick(const Skeleton& skeleton, float64 deltaTimeInSeconds) = 0;
		virtual void Reset() = 0;

		virtual void MatchDuration(float64 durationInSeconds) = 0;

		virtual float64 GetDurationInSeconds()  const	= 0;
		virtual float64 GetLocalTimeInSeconds() const	= 0;

		virtual const TArray<SQT>& GetResult() const = 0;

	protected:
		AnimationState* m_pParent;
	};

	/*
	* ClipTrigger
	*/

	class ClipNode;
	class AnimationGraph;

	struct ClipTrigger
	{
		using TriggerFunc = std::function<void(const ClipNode&, AnimationGraph&)>;

		inline ClipTrigger()
			: TriggerAt(0.0)
			, Func()
			, IsTriggered(false)
		{
		}

		inline ClipTrigger(float64 triggerAt, TriggerFunc func)
			: TriggerAt(triggerAt)
			, Func(func)
			, IsTriggered(false)
		{
		}

		float64		TriggerAt;
		TriggerFunc Func;
		bool		IsTriggered;
	};

	/*
	* ClipNode
	*/

	class ClipNode : public AnimationNode
	{
	public:
		ClipNode(AnimationState* pParent, GUID_Lambda animationGUID, float64 playbackSpeed, bool isLooping = true);
		~ClipNode() = default;

		virtual void Tick(const Skeleton& skeleton, float64 deltaTimeInSeconds) override;

		virtual void Reset() override
		{
			m_RunningTime			= 0.0;
			m_NormalizedTime		= 0.0;
			m_LocalTimeInSeconds	= 0.0;
			m_IsPlaying				= false;

			ResetTriggers();
		}

		virtual void MatchDuration(float64 durationInSeconds)
		{
			const float32 newSpeed = (m_DurationInSeconds / m_OrignalPlaybackSpeed) / fabs(durationInSeconds);
			m_PlaybackSpeed = newSpeed;
		}

		virtual const TArray<SQT>& GetResult() const override
		{
			return m_FrameData;
		}

		virtual float64 GetDurationInSeconds() const override
		{
			return m_DurationInSeconds / m_PlaybackSpeed;
		}

		virtual float64 GetLocalTimeInSeconds() const override
		{
			return m_LocalTimeInSeconds;
		}

		FORCEINLINE void AddTrigger(const ClipTrigger& trigger)
		{
			m_Triggers.EmplaceBack(trigger);
		}

		FORCEINLINE void SetIsLooping(bool isLooping)
		{
			m_IsLooping = isLooping;
		}

		FORCEINLINE void SetNumLoops(uint32 numLoops)
		{
			m_NumLoops = numLoops;
		}

		FORCEINLINE float64 GetRunningTime() const
		{
			return m_RunningTime;
		}

		FORCEINLINE Animation* GetAnimation() const
		{
			return m_pAnimation;
		}

		FORCEINLINE float64 GetPlaybackSpeed() const
		{
			return m_PlaybackSpeed;
		}

		FORCEINLINE float64 GetNormalizedTime() const
		{
			return m_NormalizedTime;
		}

		FORCEINLINE bool IsPlaying() const
		{
			return m_IsPlaying;
		}

		FORCEINLINE bool IsLooping() const
		{
			return m_IsLooping;
		}

	private:
		glm::vec3 SamplePosition(Animation::Channel& channel, float64 time);
		glm::vec3 SampleScale(Animation::Channel& channel, float64 time);
		glm::quat SampleRotation(Animation::Channel& channel, float64 time);

		void OnLoopFinish();
		
		FORCEINLINE bool IsLoopFinished()
		{
			return m_LoopFinished;
		}

		FORCEINLINE void ResetTriggers()
		{
			for (ClipTrigger& trigger : m_Triggers)
			{
				trigger.IsTriggered = false;
			}
		}

	private:
		GUID_Lambda	m_AnimationGUID;
		Animation*	m_pAnimation;

		bool m_IsLooping;
		bool m_IsPlaying;
		bool m_LoopFinished;

		uint32	m_NumLoops;
		float32	m_OrignalPlaybackSpeed;
		float32	m_PlaybackSpeed;
		float64	m_RunningTime;
		float64	m_NormalizedTime;
		float64	m_LocalTimeInSeconds;
		float64	m_DurationInSeconds;

		TArray<ClipTrigger> m_Triggers;
		TArray<SQT> m_FrameData;
	};

	/*
	* OutputNode
	*/

	class OutputNode : public AnimationNode
	{
	public:
		// The only node that can have a nullptr input
		inline OutputNode(AnimationState* pParent, AnimationNode* pIn)
			: AnimationNode(pParent)
			, m_pIn(pIn)
		{
		}

		~OutputNode() = default;

		virtual void Tick(const Skeleton& skeleton, float64 deltaTimeInSeconds) override
		{
			VALIDATE(m_pIn != nullptr);
			m_pIn->Tick(skeleton, deltaTimeInSeconds);
		}

		virtual void Reset() override
		{
			VALIDATE(m_pIn != nullptr);
			m_pIn->Reset();
		}

		virtual void MatchDuration(float64 durationInSeconds)
		{
			VALIDATE(m_pIn != nullptr);
			m_pIn->MatchDuration(durationInSeconds);
		}

		virtual const TArray<SQT>& GetResult() const override
		{
			VALIDATE(m_pIn != nullptr);
			return m_pIn->GetResult();
		}

		virtual float64 GetDurationInSeconds() const override
		{
			VALIDATE(m_pIn != nullptr);
			return m_pIn->GetDurationInSeconds();
		}

		virtual float64 GetLocalTimeInSeconds() const override
		{
			VALIDATE(m_pIn != nullptr);
			return m_pIn->GetLocalTimeInSeconds();
		}

		FORCEINLINE AnimationNode* GetInputNode() const
		{
			return m_pIn;
		}

		FORCEINLINE void SetInputNode(AnimationNode* pIn) 
		{
			m_pIn = pIn;
		}

	private:
		AnimationNode* m_pIn;
	};

	/*
	* BlendInfo - Clip0 is the leader clip, this means that it is the mainclip and is the clip that clip1 
	* will sync to if this is enabled. This also means that clip0 always will be used, so when using the cliplimit, 
	* the joints that are specified to not be used in clip1 will fallback to the same joints in clip0.
	*/

	struct BlendInfo
	{
		inline BlendInfo()
			: WeightIn1(0.5f)
			, ClipLimit1("")
			, ShouldSync(false)
		{
		}

		inline explicit BlendInfo(float32 weightIn1, PrehashedString clipLimit1 = "", bool shouldSync = false)
			: WeightIn1(weightIn1)
			, ClipLimit1(clipLimit1)
			, ShouldSync(shouldSync)
		{
		}

		bool ShouldSync;
		float32 WeightIn1;
		PrehashedString ClipLimit1; // Limit animation from this joint for clip1
	};

	/*
	* BlendNode
	*/

	class BlendNode : public AnimationNode
	{
	public:
		BlendNode(AnimationState* pParent, AnimationNode* pIn0, AnimationNode* pIn1, const BlendInfo& blendInfo);
		~BlendNode() = default;

		virtual void Tick(const Skeleton& skeleton, float64 deltaTimeInSeconds) override final;

		virtual void Reset() override
		{
			m_pIn0->Reset();
			m_pIn1->Reset();
		}

		virtual void MatchDuration(float64 durationInSeconds)
		{
			m_pIn0->MatchDuration(durationInSeconds);
			m_pIn1->MatchDuration(durationInSeconds);
		}

		virtual const TArray<SQT>& GetResult() const override
		{
			return m_FrameData;
		}

		virtual float64 GetDurationInSeconds() const override
		{
			const float64 in0 = m_pIn0->GetDurationInSeconds();
			const float64 in1 = m_pIn1->GetDurationInSeconds();
			return std::max<float64>(in0, in1);
		}

		virtual float64 GetLocalTimeInSeconds() const override
		{
			const float64 in0 = m_pIn0->GetLocalTimeInSeconds();
			const float64 in1 = m_pIn1->GetLocalTimeInSeconds();
			return std::max<float64>(in0, in1);
		}

	private:
		bool FindLimit(const Skeleton& skeleton, JointIndexType parentID, JointIndexType clipLimit);

	private:
		AnimationNode* m_pIn0;
		AnimationNode* m_pIn1;
		BlendInfo m_BlendInfo;
		TArray<SQT> m_In0;
		TArray<SQT> m_In1;
		TArray<SQT> m_FrameData;
	};
}