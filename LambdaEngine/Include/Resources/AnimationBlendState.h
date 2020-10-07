#pragma once
#include "Mesh.h"

#define INFINITE_LOOPS -1

namespace LambdaEngine
{
	/*
	* EAnimationBlendType
	*/

	enum class EAnimationBlendType
	{
		ANIMATION_BLEND_TYPE_STATIC		= 0,
		ANIMATION_BLEND_TYPE_TIME_LERP	= 1,
	};

	/*
	* AnimationBlendInfo
	*/

	struct AnimationBlendInfo
	{
		inline AnimationBlendInfo()
			: AnimationName()
			, StaticWeight(0.0f)
			, NormalizedWeight(0.0f)
		{
		}

		inline AnimationBlendInfo(const String& animationName)
			: AnimationName(animationName)
			, StaticWeight(1.0f)
			, NormalizedWeight(0.0f)
		{
		}

		inline AnimationBlendInfo(const String& animationName, float32 weight)
			: AnimationName(animationName)
			, StaticWeight(weight)
			, NormalizedWeight(0.0f)
		{
		}

		String	AnimationName;
		float32	StaticWeight;
		float32	NormalizedWeight;
	};

	/*
	* AnimationBlendState
	*/

	class AnimationBlendState
	{
	public:
		AnimationBlendState();
		AnimationBlendState(EAnimationBlendType blendType);
		~AnimationBlendState() = default;

		void PushBlendInfo(const AnimationBlendInfo& animationBlendInfo, bool normalizeWeights = true);
		void PushBlendInfo(AnimationBlendInfo&& animationBlendInfo, bool normalizeWeights = true);
		void PopBlendInfo(bool normalizeWeights = true);

		void CalculateWeights();
		
		FORCEINLINE EAnimationBlendType GetBlendType() const
		{
			return m_BlendType;
		}

		FORCEINLINE const String& GetName() const
		{
			return m_Name;
		}

		FORCEINLINE const AnimationBlendInfo& GetBlendInfo(uint32 index) const
		{
			return m_BlendInfos[index];
		}

		FORCEINLINE uint32 GetBlendInfoCount() const
		{
			return m_BlendInfos.GetSize();
		}

		FORCEINLINE TArray<AnimationBlendInfo>::ConstIterator begin() const
		{
			return m_BlendInfos.begin();
		}

		FORCEINLINE TArray<AnimationBlendInfo>::ConstIterator end() const
		{
			return m_BlendInfos.end();
		}

	private:
		bool	m_IsDirty = true;
		float32	m_TotalWeight;
		String	m_Name;
		EAnimationBlendType			m_BlendType;
		TArray<AnimationBlendInfo>	m_BlendInfos;
	};

	/*
	* ClipInfo
	*/

	struct ClipState
	{
		inline ClipState()
			: IsLooping(true)
			, NumLoops(INFINITE_LOOPS)
			, LocalTime(0.0f)
			, PlaybackSpeed(1.0f)
			, StartTime(0.0f)
			, AnimationGUID(GUID_NONE)
			, Name()
		{
		}

		inline ClipState(const String& name, GUID_Lambda animationGUID, float32 playbackSpeed = 1.0f)
			: IsLooping(true)
			, NumLoops(INFINITE_LOOPS)
			, LocalTime(0.0f)
			, PlaybackSpeed(playbackSpeed)
			, StartTime(0.0f)
			, AnimationGUID(animationGUID)
			, Name(name)
		{
		}

		inline void Reset()
		{
			LocalTime = 0.0f;
		}

		bool		IsLooping	= true;
		int32		NumLoops	= INFINITE_LOOPS;
		float32		LocalTime; // Normalized time
		float32		PlaybackSpeed;
		float64		StartTime; // Start time in the animation system
		GUID_Lambda	AnimationGUID;
		String		Name;
	};

	/*
	* AnimationState
	*/

	class AnimationState
	{
	public:
		AnimationState();
		AnimationState(const ClipState& clipState);
		~AnimationState() = default;

		void PushClip(const ClipState& clipState);
		void PushClip(ClipState&& clipState);
		void PopClip();

		void PushBlendState(const AnimationBlendState& animationBlendState);
		void PushBlendState(AnimationBlendState&& animationBlendState);
		void PopBlendState();

		void SetCurrentBlendState(const String& name);

		// Returns true if an animation with this name exists
		bool HasClip(const String& name) const;
		// Returns true if a blendstate with this name exists
		bool HasBlendState(const String& name) const;

		ClipState&			GetClip(const String& name);
		const ClipState&	GetClip(const String& name) const;
		
		AnimationBlendState&		GetBlendState(const String& name);
		const AnimationBlendState&	GetBlendState(const String& name) const;

		AnimationBlendState&		GetCurrentBlendState();
		const AnimationBlendState&	GetCurrentBlendState() const;

	private:
		TArray<ClipState>			m_Clips;
		TArray<AnimationBlendState>	m_BlendStates;
		int32	m_CurrentBlendStateIndex;
		float32	m_TotalWeight;
	};
}