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

	class AnimationNode
	{
	public:
		AnimationNode() = default;
		virtual ~AnimationNode() = default;

		virtual void Tick(const Skeleton& skeleton, float64 deltaTimeInSeconds) = 0;
		virtual void Reset() = 0;

		virtual float64 GetDurationInSeconds() const = 0;

		virtual const TArray<SQT>& GetResult() const = 0;
	};

	/*
	* ClipNode
	*/

	class ClipNode : public AnimationNode
	{
	public:
		ClipNode(GUID_Lambda animationGUID, float64 playbackSpeed, bool isLooping = true);
		~ClipNode() = default;

		virtual void Tick(const Skeleton& skeleton, float64 deltaTimeInSeconds) override;

		virtual void Reset() override
		{
			m_IsPlaying		 = false;
			m_RunningTime	 = 0.0;
			m_NormalizedTime = 0.0;
			m_LocalTimeInSeconds = 0.0;
		}

		virtual const TArray<SQT>& GetResult() const override
		{
			return m_FrameData;
		}

		virtual float64 GetDurationInSeconds() const override
		{
			return m_DurationInSeconds;
		}

		FORCEINLINE void SetIsLooping(bool isLooping)
		{
			m_IsLooping = isLooping;
		}

		FORCEINLINE void SetNumLoops(uint32 numLoops)
		{
			m_NumLoops = numLoops;
		}

	private:
		glm::vec3 SamplePosition(Animation::Channel& channel, float64 time);
		glm::vec3 SampleScale(Animation::Channel& channel, float64 time);
		glm::quat SampleRotation(Animation::Channel& channel, float64 time);

	private:
		GUID_Lambda	m_AnimationGUID;
		Animation*	m_pAnimation;

		bool m_IsLooping;
		bool m_IsPlaying;

		uint32	m_NumLoops;
		float32	m_PlaybackSpeed;
		float64	m_RunningTime;
		float64	m_NormalizedTime;
		float64	m_LocalTimeInSeconds;
		float64	m_DurationInSeconds;

		TArray<SQT> m_FrameData;
	};

	/*
	* OutputNode
	*/

	class OutputNode : public AnimationNode
	{
	public:
		// The only node that can have a nullptr input
		inline OutputNode(AnimationNode* pIn)
			: AnimationNode()
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
	* BlendInfo
	*/

	struct BlendInfo
	{
		inline BlendInfo()
			: WeightIn0(0.5f)
			, ClipLimit0("")
		{
		}

		inline explicit BlendInfo(float32 weightIn0, PrehashedString clipLimit0 = "")
			: WeightIn0(weightIn0)
			, ClipLimit0(clipLimit0)
		{
		}

		float32 WeightIn0;
		PrehashedString ClipLimit0; // Limit animation from this joint for clip0
	};

	/*
	* BlendNode
	*/

	class BlendNode : public AnimationNode
	{
	public:
		BlendNode(AnimationNode* pIn0, AnimationNode* pIn1, const BlendInfo& blendInfo);
		~BlendNode() = default;

		virtual void Tick(const Skeleton& skeleton, float64 deltaTimeInSeconds) override final;

		virtual void Reset() override
		{
			m_pIn0->Reset();
			m_pIn1->Reset();
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