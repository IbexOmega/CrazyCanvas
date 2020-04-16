#include "WaveFormat.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int LoadWaveSoundBuffer(const char* Filename, uint8_t** OutputBuffer, WaveHeader* Header)
{
    FILE* File = fopen(Filename, "rb");
    if (File != NULL)
    {
        memset(Header, 0, sizeof(WaveHeader));

        // Riff
        int32_t Read = fread(&Header->RIFF, 1, sizeof(Header->RIFF), File);
        printf("Read=%d, Riff Marker=%s\n", Read, Header->RIFF);

        // Size
        uint8_t Buffer[4];
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 4, File);
        Header->SizeInBytes = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
        printf("Read=%d, Size: %u bytes, %u kB\n", Read, Header->SizeInBytes, Header->SizeInBytes / 1024);

        // Wave
        Read = fread(&Header->WAVE, 1, sizeof(Header->WAVE), File);
        printf("Read=%d, Wave Marker=%s\n", Read, Header->WAVE);

        // Format
        Read = fread(&Header->FormatChunkMarker, 1, sizeof(Header->FormatChunkMarker), File);
        printf("Read=%d, FormatChunkMarker=%s\n", Read, Header->FormatChunkMarker);

        //Format size
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 4, File);
        Header->FormatLength = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
        printf("Read=%d, FormatLength: %u bytes, %u kB\n", Read, Header->FormatLength, Header->FormatLength / 1024);

        // Format type
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 2, File);
        Header->FormatType = Buffer[0] | (Buffer[1] << 8);

        char FormatName[10] = "";
        if (Header->FormatType == 1)
        {
            strcpy(FormatName,"PCM");
        }
        else if (Header->FormatType == 6)
        {
            strcpy(FormatName, "A-law");
        }
        else if (Header->FormatType == 7)
        {
            strcpy(FormatName, "Mu-law");
        }
        printf("Read=%d, Format type: %u, Format name: %s\n", Read, Header->FormatType, FormatName);

        // Channels
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 2, File);
        Header->Channels = Buffer[0] | (Buffer[1] << 8);
        printf("Read=%d, Channels: %u\n", Read, Header->Channels);

        // Sample Rate
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 4, File);
        Header->SampleRate = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
        printf("Read=%d, Sample rate: %u\n", Read, Header->SampleRate);

        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 4, File);
        Header->ByteRate = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
        printf("Read=%d, Byte Rate: %u, Bit Rate:%u\n", Read, Header->ByteRate, Header->ByteRate * 8);

        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 2, File);
        Header->BlockAlignment = Buffer[0] | (Buffer[1] << 8);
        printf("Read=%d, Block Alignment: %u \n", Read, Header->BlockAlignment);

        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 2, File);
        Header->BitsPerSample = Buffer[0] | (Buffer[1] << 8);
        printf("Read=%d, Bits per sample: %u \n", Read, Header->BitsPerSample);

        Read = fread(Header->DataChunkHeader, 1, sizeof(Header->DataChunkHeader), File);
        printf("Read=%d, Data Marker: %s \n", Read, Header->DataChunkHeader);

        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 4, File);
        Header->DataSize = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24 );
        printf("Read=%d, Size of data chunk: %u \n", Read, Header->DataSize);

        uint32_t SampleCount = (8 * Header->DataSize) / (Header->Channels * Header->BitsPerSample);
        printf("SampleCount=%u\n", SampleCount);

        uint32_t SampleSize = (Header->Channels * Header->BitsPerSample) / 8;
        printf("Size per sample=%u bytes\n", SampleSize);

        float DurationInSeconds = (float)Header->SizeInBytes / (float)Header->ByteRate;
        printf("Duration in seconds=%.4f\n", DurationInSeconds);

        // Make sure that the bytes-per-sample is completely divisible by num.of channels
        uint32_t BytesInEachChannel = (SampleSize / Header->Channels);
        if ((BytesInEachChannel * Header->Channels) != SampleSize)
        {
            printf("Error: %u * %u != %u\n", BytesInEachChannel, Header->Channels, SampleSize);
            return WAVE_ERROR;
        }

        if (Header->FormatType != 1)
        {
            printf("Error: Unsupported format\n");
            return WAVE_ERROR;
        }

        int32_t LowLimit   = 0;
		int32_t HighLimit  = 0;
        switch (Header->BitsPerSample) 
        {
            case 8:
                LowLimit    = -128;
                HighLimit   = 127;
                break;
            case 16:
                LowLimit    = -32768;
                HighLimit   = 32767;
                break;
            case 32:
                LowLimit    = -2147483648;
                HighLimit   = 2147483647;
                break;
            default:
                printf("Error: Unsupported BitsPerSample=%u\n", Header->BitsPerSample);
                return WAVE_ERROR;
        }		

        printf("Low: %d, High: %d\n", LowLimit, HighLimit);

        const uint32_t BufferSize = SampleSize * SampleCount;
        (*OutputBuffer) = (uint8_t*)malloc(BufferSize);
        Read = fread((*OutputBuffer), 1, BufferSize, File);
        
        printf("Bytes read: %d, SizeOfStream: %u\n", Read, BufferSize);

        fclose(File);
        return WAVE_SUCCESS;
    }
    else
    {
        return WAVE_ERROR;
    }
}