#pragma once

#include "Rendering/Core/API/GraphicsTypes.h"
#include "NsRender/RenderDevice.h"

namespace LambdaEngine
{
	const uint32 NOESIS_VF_POS		= 0;
	const uint32 NOESIS_VF_COLOR	= 1;
	const uint32 NOESIS_VF_TEX0		= 2;
	const uint32 NOESIS_VF_TEX1		= 4;
	const uint32 NOESIS_VF_TEX2		= 8;
	const uint32 NOESIS_VF_COVERAGE	= 16;

	struct NoesisShaderData
	{
		uint8	VertexFormat	= 0;
		uint32	VertexShaderID	= 0;
		uint32	PixelShaderID	= 0;
	};

	FORCEINLINE EFormat NoesisFormatToLamdaFormat(Noesis::TextureFormat::Enum format)
	{
		switch (format)
		{
		case Noesis::TextureFormat::Enum::R8:		return EFormat::FORMAT_R8_UNORM;
		case Noesis::TextureFormat::Enum::RGBA8:	return EFormat::FORMAT_R8G8B8A8_UNORM;
		}

		return EFormat::FORMAT_R8G8B8A8_UNORM;
	}

	FORCEINLINE NoesisShaderData NoesisGetShaderData(uint32 n)
	{
		NoesisShaderData s = {};

		switch (n)
		{
		case Noesis::Shader::RGBA:						s = { NOESIS_VF_POS,														0, 0 };		break;
		case Noesis::Shader::Mask:						s = { NOESIS_VF_POS,														0, 1 };		break;
		case Noesis::Shader::Path_Solid:				s = { NOESIS_VF_POS | NOESIS_VF_COLOR,										1, 2 };		break;
		case Noesis::Shader::Path_Linear:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0,										2, 3 };		break;
		case Noesis::Shader::Path_Radial:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0,										2, 4 };		break;
		case Noesis::Shader::Path_Pattern:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0,										2, 5 };		break;

		case Noesis::Shader::PathAA_Solid:				s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_COVERAGE,					3, 6 };		break;
		case Noesis::Shader::PathAA_Linear:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_COVERAGE,					4, 7 };		break;
		case Noesis::Shader::PathAA_Radial:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_COVERAGE,					4, 8 };		break;
		case Noesis::Shader::PathAA_Pattern:			s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_COVERAGE,					4, 9 };		break;

		case Noesis::Shader::SDF_Solid:					s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1,						9, 10 };	break;
		case Noesis::Shader::SDF_Linear:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1,						10, 11 };	break;
		case Noesis::Shader::SDF_Radial:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1,						10, 12 };	break;
		case Noesis::Shader::SDF_Pattern:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1,						10, 13 };	break;

		case Noesis::Shader::Image_Opacity_Solid:		s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1,						5, 14 };	break;
		case Noesis::Shader::Image_Opacity_Linear:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1,						6, 15 };	break;
		case Noesis::Shader::Image_Opacity_Radial:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1,						6, 16 };	break;
		case Noesis::Shader::Image_Opacity_Pattern:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1,						6, 17 };	break;

		case Noesis::Shader::Image_Shadow35V:			s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	7, 18 };	break;
		case Noesis::Shader::Image_Shadow63V:			s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	7, 19 };	break;
		case Noesis::Shader::Image_Shadow127V:			s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	7, 20 };	break;

		case Noesis::Shader::Image_Shadow35H_Solid:		s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	7, 21 };	break;
		case Noesis::Shader::Image_Shadow35H_Linear:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 22 };	break;
		case Noesis::Shader::Image_Shadow35H_Radial:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 23 };	break;
		case Noesis::Shader::Image_Shadow35H_Pattern:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 24 };	break;

		case Noesis::Shader::Image_Shadow63H_Solid:		s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	7, 25 };	break;
		case Noesis::Shader::Image_Shadow63H_Linear:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 26 };	break;
		case Noesis::Shader::Image_Shadow63H_Radial:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 27 };	break;
		case Noesis::Shader::Image_Shadow63H_Pattern:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 28 };	break;

		case Noesis::Shader::Image_Shadow127H_Solid:	s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	7, 29 };	break;
		case Noesis::Shader::Image_Shadow127H_Linear:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 30 };	break;
		case Noesis::Shader::Image_Shadow127H_Radial:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 31 };	break;
		case Noesis::Shader::Image_Shadow127H_Pattern:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 32 };	break;

		case Noesis::Shader::Image_Blur35V:				s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	7, 33 };	break;
		case Noesis::Shader::Image_Blur63V:				s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	7, 34 };	break;
		case Noesis::Shader::Image_Blur127V:			s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	7, 35 };	break;

		case Noesis::Shader::Image_Blur35H_Solid:		s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	7, 36 };	break;
		case Noesis::Shader::Image_Blur35H_Linear:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 37 };	break;
		case Noesis::Shader::Image_Blur35H_Radial:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 38 };	break;
		case Noesis::Shader::Image_Blur35H_Pattern:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 39 };	break;

		case Noesis::Shader::Image_Blur63H_Solid:		s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	7, 40 };	break;
		case Noesis::Shader::Image_Blur63H_Linear:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 41 };	break;
		case Noesis::Shader::Image_Blur63H_Radial:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 42 };	break;
		case Noesis::Shader::Image_Blur63H_Pattern:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 43 };	break;

		case Noesis::Shader::Image_Blur127H_Solid:		s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	7, 44 };	break;
		case Noesis::Shader::Image_Blur127H_Linear:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 45 };	break;
		case Noesis::Shader::Image_Blur127H_Radial:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 46 };	break;
		case Noesis::Shader::Image_Blur127H_Pattern:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		8, 47 };	break;

		default: VALIDATE(false);
		}

		return s;
	}
}