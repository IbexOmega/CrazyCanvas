#pragma once

#include "Rendering/Core/API/GraphicsTypes.h"
#include "NsRender/RenderDevice.h"

namespace LambdaEngine
{
	const uint8 NOESIS_VF_POS		= 0;
	const uint8 NOESIS_VF_COLOR		= 1;
	const uint8 NOESIS_VF_TEX0		= 2;
	const uint8 NOESIS_VF_TEX1		= 4;
	const uint8 NOESIS_VF_TEX2		= 8;
	const uint8 NOESIS_VF_COVERAGE	= 16;

	struct NoesisShaderData
	{
		uint8	VertexFormat	= 0;
		uint8	VertexSize		= 0;
		uint32	VertexShaderID	= 0;
		uint32	PixelShaderID	= 0;
	};

	FORCEINLINE EFormat NoesisFormatToLambdaFormat(Noesis::TextureFormat::Enum format)
	{
		switch (format)
		{
		case Noesis::TextureFormat::Enum::R8:		return EFormat::FORMAT_R8_UNORM;
		case Noesis::TextureFormat::Enum::RGBA8:	return EFormat::FORMAT_R8G8B8A8_UNORM;
		}

		return EFormat::FORMAT_R8G8B8A8_UNORM;
	}

	FORCEINLINE bool NoesisStencilOpToLambdaStencilOp(uint8 stencilOp, EStencilOp& outStencilOp)
	{
		switch (stencilOp)
		{
		case Noesis::StencilMode::Disabled:		return false;
		case Noesis::StencilMode::Equal_Keep:	outStencilOp = EStencilOp::STENCIL_OP_KEEP; return true;
		case Noesis::StencilMode::Equal_Incr:	outStencilOp = EStencilOp::STENCIL_OP_INCREMENT_AND_WRAP; return true;
		case Noesis::StencilMode::Equal_Decr:	outStencilOp = EStencilOp::STENCIL_OP_DECREMENT_AND_WRAP; return true;
		}

		return false;
	}

	FORCEINLINE NoesisShaderData NoesisGetShaderData(uint32 n)
	{
		NoesisShaderData s = {};

		//Sizes:
		// NOESIS_VF_POS		= 8
		// NOESIS_VF_COLOR		= 4
		// NOESIS_VF_TEX0		= 8
		// NOESIS_VF_TEX1		= 8
		// NOESIS_VF_TEX2		= 8
		// NOESIS_VF_COVERAGE	= 4

		switch (n)
		{
		case Noesis::Shader::RGBA:						s = { NOESIS_VF_POS,														8,	0, 0 };		break;
		case Noesis::Shader::Mask:						s = { NOESIS_VF_POS,														8,	0, 1 };		break;
		case Noesis::Shader::Path_Solid:				s = { NOESIS_VF_POS | NOESIS_VF_COLOR,										12,	1, 2 };		break;
		case Noesis::Shader::Path_Linear:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0,										16,	2, 3 };		break;
		case Noesis::Shader::Path_Radial:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0,										16,	2, 4 };		break;
		case Noesis::Shader::Path_Pattern:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0,										16,	2, 5 };		break;

		case Noesis::Shader::PathAA_Solid:				s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_COVERAGE,					16,	3, 6 };		break;
		case Noesis::Shader::PathAA_Linear:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_COVERAGE,					20,	4, 7 };		break;
		case Noesis::Shader::PathAA_Radial:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_COVERAGE,					20,	4, 8 };		break;
		case Noesis::Shader::PathAA_Pattern:			s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_COVERAGE,					20,	4, 9 };		break;

		case Noesis::Shader::SDF_Solid:					s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1,						20,	9, 10 };	break;
		case Noesis::Shader::SDF_Linear:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1,						24,	10, 11 };	break;
		case Noesis::Shader::SDF_Radial:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1,						24,	10, 12 };	break;
		case Noesis::Shader::SDF_Pattern:				s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1,						24,	10, 13 };	break;

		case Noesis::Shader::Image_Opacity_Solid:		s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1,						20,	5, 14 };	break;
		case Noesis::Shader::Image_Opacity_Linear:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1,						24,	6, 15 };	break;
		case Noesis::Shader::Image_Opacity_Radial:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1,						24,	6, 16 };	break;
		case Noesis::Shader::Image_Opacity_Pattern:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1,						24,	6, 17 };	break;

		case Noesis::Shader::Image_Shadow35V:			s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	28,	7, 18 };	break;
		case Noesis::Shader::Image_Shadow63V:			s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	28,	7, 19 };	break;
		case Noesis::Shader::Image_Shadow127V:			s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	28,	7, 20 };	break;

		case Noesis::Shader::Image_Shadow35H_Solid:		s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	28,	7, 21 };	break;
		case Noesis::Shader::Image_Shadow35H_Linear:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 22 };	break;
		case Noesis::Shader::Image_Shadow35H_Radial:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 23 };	break;
		case Noesis::Shader::Image_Shadow35H_Pattern:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 24 };	break;

		case Noesis::Shader::Image_Shadow63H_Solid:		s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	28,	7, 25 };	break;
		case Noesis::Shader::Image_Shadow63H_Linear:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 26 };	break;
		case Noesis::Shader::Image_Shadow63H_Radial:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 27 };	break;
		case Noesis::Shader::Image_Shadow63H_Pattern:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 28 };	break;

		case Noesis::Shader::Image_Shadow127H_Solid:	s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	28,	7, 29 };	break;
		case Noesis::Shader::Image_Shadow127H_Linear:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 30 };	break;
		case Noesis::Shader::Image_Shadow127H_Radial:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 31 };	break;
		case Noesis::Shader::Image_Shadow127H_Pattern:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 32 };	break;

		case Noesis::Shader::Image_Blur35V:				s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	28,	7, 33 };	break;
		case Noesis::Shader::Image_Blur63V:				s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	28,	7, 34 };	break;
		case Noesis::Shader::Image_Blur127V:			s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	28,	7, 35 };	break;

		case Noesis::Shader::Image_Blur35H_Solid:		s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	28,	7, 36 };	break;
		case Noesis::Shader::Image_Blur35H_Linear:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 37 };	break;
		case Noesis::Shader::Image_Blur35H_Radial:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 38 };	break;
		case Noesis::Shader::Image_Blur35H_Pattern:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 39 };	break;

		case Noesis::Shader::Image_Blur63H_Solid:		s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	28,	7, 40 };	break;
		case Noesis::Shader::Image_Blur63H_Linear:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 41 };	break;
		case Noesis::Shader::Image_Blur63H_Radial:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 42 };	break;
		case Noesis::Shader::Image_Blur63H_Pattern:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 43 };	break;

		case Noesis::Shader::Image_Blur127H_Solid:		s = { NOESIS_VF_POS | NOESIS_VF_COLOR | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,	28,	7, 44 };	break;
		case Noesis::Shader::Image_Blur127H_Linear:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 45 };	break;
		case Noesis::Shader::Image_Blur127H_Radial:		s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 46 };	break;
		case Noesis::Shader::Image_Blur127H_Pattern:	s = { NOESIS_VF_POS | NOESIS_VF_TEX0 | NOESIS_VF_TEX1 | NOESIS_VF_TEX2,		32,	8, 47 };	break;

		default: VALIDATE(false);
		}

		return s;
	}
}