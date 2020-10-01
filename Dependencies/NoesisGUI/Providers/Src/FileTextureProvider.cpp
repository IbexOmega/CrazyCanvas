////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/FileTextureProvider.h>
#include <NsCore/Error.h>
#include <NsCore/Log.h>
#include <NsCore/Ptr.h>
#include <NsDrawing/Color.h>
#include <NsRender/RenderDevice.h>
#include <NsRender/Texture.h>
#include <NsGui/Stream.h>


NS_WARNING_PUSH

#ifdef _PREFAST_
#include <codeanalysis/warnings.h>
NS_MSVC_WARNING_DISABLE(ALL_CODE_ANALYSIS_WARNINGS)
#endif

NS_MSVC_WARNING_DISABLE(4244 4242 4100)
NS_GCC_CLANG_WARNING_DISABLE("-Wconversion")
NS_GCC_CLANG_WARNING_DISABLE("-Wunused-function")
#if NS_CLANG_HAS_WARNING("-Wcomma")
NS_CLANG_WARNING_DISABLE("-Wcomma")
#endif

#define STBI_MALLOC(sz) Noesis::Alloc(sz)
#define STBI_REALLOC(p,sz) Noesis::Realloc(p, sz)
#define STBI_FREE(p) Noesis::Dealloc(p)
#define STBI_ASSERT(x) NS_ASSERT(x)
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

NS_WARNING_POP

// The compiler can only determine unreferenced functions after it finished parsing the compiled
// source file, so we cannot disable it only for the header include
NS_MSVC_WARNING_DISABLE(4505)


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
static int Read(void *user, char *data, int size)
{
    Stream* stream = (Stream*)user;
    return stream->Read(data, size);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static void Skip(void *user, int n)
{
    Stream* stream = (Stream*)user;
    stream->SetPosition((int)stream->GetPosition() + n);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static int Eof(void *user)
{
    Stream* stream = (Stream*)user;
    return stream->GetPosition() >= stream->GetLength();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
FileTextureProvider::FileTextureProvider()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TextureInfo FileTextureProvider::GetTextureInfo(const char* filename)
{
    TextureInfo info = { 0, 0 };

    Ptr<Stream> file = OpenStream(filename);
    if (file != 0)
    {
        int x, y, n;
        stbi_io_callbacks callbacks = { Read, Skip, Eof };
        if (stbi_info_from_callbacks(&callbacks, file, &x, &y, &n))
        {
            info.width = x;
            info.height = y;
        }
        else
        {
            NS_LOG_WARNING("%s: %s", filename, stbi_failure_reason());
        }

        file->Close();
    }

    return info;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Texture> FileTextureProvider::LoadTexture(const char* filename, RenderDevice* device)
{
    Ptr<Stream> file = OpenStream(filename);
    if (file == 0)
    {
        return nullptr;
    }

    int x, y, n;
    stbi_io_callbacks callbacks = { Read, Skip, Eof };
    stbi_uc* img = stbi_load_from_callbacks(&callbacks, file, &x, &y, &n, 4);
    if (img == 0)
    {
        NS_LOG_WARNING("%s: %s", filename, stbi_failure_reason());
        return nullptr;
    }

    // Premultiply alpha
    if (n == 4)
    {
        if (device->GetCaps().linearRendering)
        {
            for (int i = 0; i < x * y; i++)
            {
                float a = img[4 * i + 3] / 255.0f;
                float r = LinearToSRGB(SRGBToLinear(img[4 * i + 0] / 255.0f) * a);
                float g = LinearToSRGB(SRGBToLinear(img[4 * i + 1] / 255.0f) * a);
                float b = LinearToSRGB(SRGBToLinear(img[4 * i + 2] / 255.0f) * a);

                img[4 * i + 0] = (stbi_uc)Clip(Trunc(r * 255.0f), 0, 255);
                img[4 * i + 1] = (stbi_uc)Clip(Trunc(g * 255.0f), 0, 255);
                img[4 * i + 2] = (stbi_uc)Clip(Trunc(b * 255.0f), 0, 255);
            }
        }
        else
        {
            for (int i = 0; i < x * y; i++)
            {
                stbi_uc a = img[4 * i + 3];
                img[4 * i + 0] = (stbi_uc)(((uint32_t)img[4 * i] * a) / 255);
                img[4 * i + 1] = (stbi_uc)(((uint32_t)img[4 * i + 1] * a) / 255);
                img[4 * i + 2] = (stbi_uc)(((uint32_t)img[4 * i + 2] * a) / 255);
            }
        }
    }

    const void* data[] = { img };
    Ptr<Texture> texture = device->CreateTexture(filename, x, y, 1, TextureFormat::RGBA8, data);
    stbi_image_free(img);

    file->Close();

    return texture;
}
