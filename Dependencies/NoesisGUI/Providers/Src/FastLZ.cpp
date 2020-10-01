////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/FastLZ.h>
#include <NsCore/Error.h>

#include <string.h>


#define MAX_L2_DISTANCE 8191
#define MAGIC 0x4b50534e


using namespace NoesisApp;


struct Header
{
    uint32_t magic;
    uint32_t size;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
static void FastLZMove(uint8_t* dest, const uint8_t* src, uint32_t count)
{
    if ((count > 4) && (dest >= src + count))
    {
        memmove(dest, src, count);
    }
    else
    {
        switch (count)
        {
        default:
            do { *dest++ = *src++; } while (--count);
            break;
        case 3:
            *dest++ = *src++;
        case 2:
            *dest++ = *src++;
        case 1:
            *dest++ = *src++;
        case 0:
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t FastLZ::DecompressBufferSize(const void* buffer)
{
    Header* header = (Header*)buffer;
    NS_ASSERT(header->magic == MAGIC);
    return header->size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void FastLZ::Decompress(const void* input, uint32_t length, void* output)
{
    Header* header = (Header*)input;
    NS_ASSERT(header->magic == MAGIC);
    int maxout = header->size;

    const uint8_t* ip = (const uint8_t*)input + sizeof(Header);
    const uint8_t* ip_limit = ip + length - sizeof(Header);
    const uint8_t* ip_bound = ip_limit - 2;
    uint8_t* op = (uint8_t*)output;
    uint8_t* op_limit = op + maxout;
    uint32_t ctrl = (*ip++) & 31;

    while (1)
    {
        if (ctrl >= 32)
        {
            uint32_t len = (ctrl >> 5) - 1;
            uint32_t ofs = (ctrl & 31) << 8;
            const uint8_t* ref = op - ofs - 1;

            uint8_t code;
            if (len == 7 - 1)
            {
                do
                {
                    NS_ASSERT(ip <= ip_bound);
                    code = *ip++;
                    len += code;
                }
                while (code == 255);
            }

            code = *ip++;
            ref -= code;
            len += 3;

            /* match from 16-bit distance */
            if (NS_UNLIKELY(code == 255))
            {
                if (NS_LIKELY(ofs == (31 << 8)))
                {
                    NS_ASSERT(ip < ip_bound);
                    ofs = (*ip++) << 8;
                    ofs += *ip++;
                    ref = op - ofs - MAX_L2_DISTANCE - 1;
                }
            }

            NS_ASSERT(op + len <= op_limit);
            NS_ASSERT(ref >= (uint8_t*)output);
            FastLZMove(op, ref, len);
            op += len;
        }
        else
        {
            ctrl++;
            NS_ASSERT(op + ctrl <= op_limit);
            NS_ASSERT(ip + ctrl <= ip_limit);
            memcpy(op, ip, ctrl);
            ip += ctrl;
            op += ctrl;
        }

        if (NS_UNLIKELY(ip >= ip_limit)) break;
        ctrl = *ip++;
    }

    NS_ASSERT((int)(op - (uint8_t*)output) == maxout);
}
