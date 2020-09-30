////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_FASTLZ_H__
#define __APP_FASTLZ_H__


#include <NsCore/Noesis.h>
#include <NsApp/ProvidersApi.h>


namespace NoesisApp
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// FastLZ - lightning-fast lossless compression library
/// https://ariya.github.io/FastLZ/
////////////////////////////////////////////////////////////////////////////////////////////////////
struct NS_APP_PROVIDERS_API FastLZ
{
    /// Returns the size needed to decompress the given block of data
    static uint32_t DecompressBufferSize(const void* buffer);

    /// Decompress a block of compressed data
    static void Decompress(const void* buffer, uint32_t length, void* output);
};

}

#endif
