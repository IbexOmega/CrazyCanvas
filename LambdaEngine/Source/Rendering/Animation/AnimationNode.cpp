#include "Rendering/Animation/AnimationNode.h"

#include "Resources/ResourceManager.h"

namespace LambdaEngine
{
	/*
	* BinaryInterpolator
	*/

	void BinaryInterpolator::Interpolate(float32 factor)
	{
		VALIDATE(In0.GetSize() == In1.GetSize());

		const uint32 size			= In0.GetSize();
		const float32 realFactor	= glm::clamp(factor, 0.0f, 1.0f);

		if (Out.GetSize() < size)
		{
			Out.Resize(size);
		}

		for (uint32 i = 0; i < size; i++)
		{
			const SQT& in0	= In0[i];
			const SQT& in1	= In1[i];
			SQT& out		= Out[i];

			out.Translation	= glm::mix(in0.Translation, in1.Translation, realFactor);
			out.Scale		= glm::mix(in0.Scale, in1.Scale, realFactor);
			out.Rotation	= glm::slerp(in0.Rotation, in1.Rotation, realFactor);
			out.Rotation	= glm::normalize(out.Rotation);
		}
	}

	/*
	* ClipNode
	*/

	ClipNode::ClipNode(GUID_Lambda animationGUID, float64 playbackSpeed, bool isLooping)
		: AnimationNode()
		, m_AnimationGUID(animationGUID)
		, m_pAnimation(nullptr)
		, m_IsLooping(isLooping)
		, m_IsPlaying(false)
		, m_NumLoops(1)
		, m_PlaybackSpeed(playbackSpeed)
		, m_RunningTime(0.0)
		, m_NormalizedTime(0.0)
		, m_LocalTimeInSeconds(0.0)
		, m_DurationInSeconds(0.0)
		, m_FrameData()
	{
		Animation* pAnimation = ResourceManager::GetAnimation(animationGUID);
		if (pAnimation)
		{
			m_DurationInSeconds = pAnimation->DurationInSeconds();
			m_pAnimation = pAnimation;
		}

		if (m_IsLooping)
		{
			m_NumLoops = INFINITE_LOOPS;
		}
	}

	void ClipNode::Tick(const Skeleton& skeleton, float64 deltaTimeInSeconds)
	{
		// Get localtime for the animation-clip
		m_RunningTime += deltaTimeInSeconds;
		float64 localTime = m_RunningTime * fabs(m_PlaybackSpeed);
		if (m_IsLooping)
		{
			if (m_NumLoops != INFINITE_LOOPS)
			{
				const float64 totalDuration = m_NumLoops * m_DurationInSeconds;
				localTime = glm::clamp(localTime, 0.0, totalDuration);
			}

			localTime = fmod(localTime, m_DurationInSeconds);
		}
		else
		{
			localTime = glm::clamp(localTime, 0.0, m_DurationInSeconds);
		}

		m_LocalTimeInSeconds = localTime;
		m_NormalizedTime = m_LocalTimeInSeconds / m_DurationInSeconds;
		if (m_PlaybackSpeed < 0.0)
		{
			m_NormalizedTime = 1.0 - m_NormalizedTime;
		}

		// Make sure we have enough matrices
		if (m_FrameData.GetSize() < skeleton.Joints.GetSize())
		{
			m_FrameData.Resize(skeleton.Joints.GetSize());
		}

		Animation& animation	= *m_pAnimation;
		const float64 timestamp	= m_NormalizedTime * animation.DurationInTicks;
		for (Animation::Channel& channel : animation.Channels)
		{
			// Retrive the bone ID
			auto it = skeleton.JointMap.find(channel.Name);
			if (it == skeleton.JointMap.end())
			{
				continue;
			}

			// Sample SQT for this animation
			glm::vec3 position = SamplePosition(channel, timestamp);
			glm::quat rotation = SampleRotation(channel, timestamp);
			glm::vec3 scale = SampleScale(channel, timestamp);

			const uint32 jointID = it->second;
			m_FrameData[jointID] = SQT(position, scale, rotation);
		}
	}

	glm::vec3 ClipNode::SamplePosition(Animation::Channel& channel, float64 time)
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

	glm::vec3 ClipNode::SampleScale(Animation::Channel& channel, float64 time)
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

	glm::quat ClipNode::SampleRotation(Animation::Channel& channel, float64 time)
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
	* BlendNode
	*/

	BlendNode::BlendNode(AnimationNode* pIn0, AnimationNode* pIn1, const BlendInfo& blendInfo)
		: AnimationNode()
		, m_pIn0(pIn0)
		, m_pIn1(pIn1)
		, m_BlendInfo(blendInfo)
	{
		VALIDATE(m_pIn0 != nullptr);
		VALIDATE(m_pIn1 != nullptr);
	}

	void BlendNode::Tick(const Skeleton& skeleton, float64 deltaTimeInSeconds)
	{
		// Tick both
		m_pIn0->Tick(skeleton, deltaTimeInSeconds);
		m_pIn1->Tick(skeleton, deltaTimeInSeconds);

		// We need to store a copy since we might change the content, and we cant change the clip data
		// What if a clip is used in mulitple states?
		m_In0 = m_pIn0->GetResult();
		m_In1 = m_pIn1->GetResult();

		VALIDATE(m_In0.GetSize() == skeleton.Joints.GetSize());

		// Make sure we do not use unwanted joints
		if (m_BlendInfo.ClipLimit0 != "")
		{
			auto joint = skeleton.JointMap.find(m_BlendInfo.ClipLimit0);
			if (joint != skeleton.JointMap.end())
			{
				JointIndexType clipLimit = joint->second;
				for (uint32 i = 0; i < m_In0.GetSize(); i++)
				{
					JointIndexType myID = static_cast<JointIndexType>(i);
					if (!FindLimit(skeleton, myID, clipLimit))
					{
						m_In0[i] = m_In1[i];
					}
				}
			}
		}

		// The inputs are swapped so that weight corresponds to correct node
		BinaryInterpolator interpolator(m_In1, m_In0, m_FrameData);
		interpolator.Interpolate(m_BlendInfo.WeightIn0);
	}
	
	bool BlendNode::FindLimit(const Skeleton& skeleton, JointIndexType parentID, JointIndexType clipLimit)
	{
		if (parentID == clipLimit)
		{
			return true;
		}
		else if (parentID == INVALID_JOINT_ID)
		{
			return false;
		}
		else
		{
			const Joint& joint = skeleton.Joints[parentID];
			return FindLimit(skeleton, joint.ParentBoneIndex, clipLimit);
		}
	}
}