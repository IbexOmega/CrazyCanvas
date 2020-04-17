#include "Audio/WaveFormat.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace LambdaEngine
{
	int32 LoadWaveSoundBuffer(const char* pFilepath, byte** ppOutputBuffer, WaveHeader* pHeader)
	{
		FILE* File = fopen(pFilepath, "rb");
		if (File != NULL)
		{
			memset(pHeader, 0, sizeof(WaveHeader));

			// Riff
			int32 Read = fread(&pHeader->RIFF, 1, sizeof(pHeader->RIFF), File);
			printf("Read=%d, Riff Marker=%s\n", Read, pHeader->RIFF);

			// Size
			uint8_t Buffer[4];
			memset(Buffer, 0, sizeof(Buffer));

			Read = fread(Buffer, 1, 4, File);
			pHeader->SizeInBytes = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
			printf("Read=%d, Size: %u bytes, %u kB\n", Read, pHeader->SizeInBytes, pHeader->SizeInBytes / 1024);

			// Wave
			Read = fread(&pHeader->WAVE, 1, sizeof(pHeader->WAVE), File);
			printf("Read=%d, Wave Marker=%s\n", Read, pHeader->WAVE);

			// Format
			Read = fread(&pHeader->FormatChunkMarker, 1, sizeof(pHeader->FormatChunkMarker), File);
			printf("Read=%d, FormatChunkMarker=%s\n", Read, pHeader->FormatChunkMarker);

			//Format size
			memset(Buffer, 0, sizeof(Buffer));

			Read = fread(Buffer, 1, 4, File);
			pHeader->FormatLength = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
			printf("Read=%d, FormatLength: %u bytes, %u kB\n", Read, pHeader->FormatLength, pHeader->FormatLength / 1024);

			// Format type
			memset(Buffer, 0, sizeof(Buffer));

			Read = fread(Buffer, 1, 2, File);
			pHeader->FormatType = Buffer[0] | (Buffer[1] << 8);

			char FormatName[10] = "";
			if (pHeader->FormatType == 1)
			{
				strcpy(FormatName, "PCM");
			}
			else if (pHeader->FormatType == 6)
			{
				strcpy(FormatName, "A-law");
			}
			else if (pHeader->FormatType == 7)
			{
				strcpy(FormatName, "Mu-law");
			}
			printf("Read=%d, Format type: %u, Format name: %s\n", Read, pHeader->FormatType, FormatName);

			// Channels
			memset(Buffer, 0, sizeof(Buffer));

			Read = fread(Buffer, 1, 2, File);
			pHeader->Channels = Buffer[0] | (Buffer[1] << 8);
			printf("Read=%d, Channels: %u\n", Read, pHeader->Channels);

			// Sample Rate
			memset(Buffer, 0, sizeof(Buffer));

			Read = fread(Buffer, 1, 4, File);
			pHeader->SampleRate = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
			printf("Read=%d, Sample rate: %u\n", Read, pHeader->SampleRate);

			memset(Buffer, 0, sizeof(Buffer));

			Read = fread(Buffer, 1, 4, File);
			pHeader->ByteRate = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
			printf("Read=%d, Byte Rate: %u, Bit Rate:%u\n", Read, pHeader->ByteRate, pHeader->ByteRate * 8);

			memset(Buffer, 0, sizeof(Buffer));

			Read = fread(Buffer, 1, 2, File);
			pHeader->BlockAlignment = Buffer[0] | (Buffer[1] << 8);
			printf("Read=%d, Block Alignment: %u \n", Read, pHeader->BlockAlignment);

			memset(Buffer, 0, sizeof(Buffer));

			Read = fread(Buffer, 1, 2, File);
			pHeader->BitsPerSample = Buffer[0] | (Buffer[1] << 8);
			printf("Read=%d, Bits per sample: %u \n", Read, pHeader->BitsPerSample);

			Read = fread(pHeader->DataChunkHeader, 1, sizeof(pHeader->DataChunkHeader), File);
			printf("Read=%d, Data Marker: %s \n", Read, pHeader->DataChunkHeader);

			memset(Buffer, 0, sizeof(Buffer));

			Read = fread(Buffer, 1, 4, File);
			pHeader->DataSize = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
			printf("Read=%d, Size of data chunk: %u \n", Read, pHeader->DataSize);

			uint32_t SampleCount = (8 * pHeader->DataSize) / (pHeader->Channels * pHeader->BitsPerSample);
			printf("SampleCount=%u\n", SampleCount);

			uint32_t SampleSize = (pHeader->Channels * pHeader->BitsPerSample) / 8;
			printf("Size per sample=%u bytes\n", SampleSize);

			float DurationInSeconds = (float)pHeader->SizeInBytes / (float)pHeader->ByteRate;
			printf("Duration in seconds=%.4f\n", DurationInSeconds);

			// Make sure that the bytes-per-sample is completely divisible by num.of channels
			uint32_t BytesInEachChannel = (SampleSize / pHeader->Channels);
			if ((BytesInEachChannel * pHeader->Channels) != SampleSize)
			{
				printf("Error: %u * %u != %u\n", BytesInEachChannel, pHeader->Channels, SampleSize);
				return WAVE_ERROR;
			}

			if (pHeader->FormatType != 1)
			{
				printf("Error: Unsupported format\n");
				return WAVE_ERROR;
			}

			int32_t LowLimit = 0;
			int32_t HighLimit = 0;
			switch (pHeader->BitsPerSample)
			{
			case 8:
				LowLimit = -128;
				HighLimit = 127;
				break;
			case 16:
				LowLimit = -32768;
				HighLimit = 32767;
				break;
			case 32:
				LowLimit = -2147483648;
				HighLimit = 2147483647;
				break;
			default:
				printf("Error: Unsupported BitsPerSample=%u\n", pHeader->BitsPerSample);
				return WAVE_ERROR;
			}

			printf("Low: %d, High: %d\n", LowLimit, HighLimit);

			const uint32_t BufferSize = SampleSize * SampleCount;
			(*ppOutputBuffer) = (uint8_t*)malloc(BufferSize);
			Read = fread((*ppOutputBuffer), 1, BufferSize, File);

			printf("Bytes read: %d, SizeOfStream: %u\n", Read, BufferSize);

			fclose(File);
			return WAVE_SUCCESS;
		}
		else
		{
			return WAVE_ERROR;
		}
	}
}