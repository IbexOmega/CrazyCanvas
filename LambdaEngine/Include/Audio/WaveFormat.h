#pragma once

#include "LambdaEngine.h"
#include <stdint.h>

#define WAVE_ERROR   (int)-1
#define WAVE_SUCCESS (int)0

namespace LambdaEngine
{
	struct WaveHeader
	{
		char        RIFF[4];
		uint32_t    SizeInBytes;
		char        WAVE[4];
		char        FormatChunkMarker[4];
		uint32_t    FormatLength;
		uint16_t    FormatType;
		uint16_t    Channels;
		uint32_t    SampleRate;
		uint32_t    ByteRate;
		uint16_t    BlockAlignment;
		uint16_t    BitsPerSample;
		char        DataChunkHeader[4];
		uint32_t    DataSize;
	};

	int32 LoadWaveSoundBuffer(const char* pFilepath, byte** ppOutputBuffer, WaveHeader* pHeader);
}