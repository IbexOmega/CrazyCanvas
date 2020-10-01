////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __APP_EMBEDDEDTEXTUREPROVIDER_H__
#define __APP_EMBEDDEDTEXTUREPROVIDER_H__


#include <NsCore/Noesis.h>
#include <NsCore/Vector.h>
#include <NsCore/ArrayRef.h>
#include <NsCore/Ptr.h>
#include <NsApp/ProvidersApi.h>
#include <NsApp/FileTextureProvider.h>


namespace NoesisApp
{

struct EmbeddedTexture
{
    const char* name;
    Noesis::ArrayRef<uint8_t> data;
};

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// A provider for textures embeded in memory
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_APP_PROVIDERS_API EmbeddedTextureProvider: public FileTextureProvider
{
public:
    EmbeddedTextureProvider(Noesis::ArrayRef<EmbeddedTexture> textures, TextureProvider* fallback = 0);

private:
    /// From TextureProvider
    //@{
    Noesis::TextureInfo GetTextureInfo(const char* uri) override;
    Noesis::Ptr<Noesis::Texture> LoadTexture(const char* uri, Noesis::RenderDevice* device) override;
    //@}

    /// From FileTextureProvider
    //@{
    Noesis::Ptr<Noesis::Stream> OpenStream(const char* uri) const override;
    //@}

private:
    Noesis::Vector<EmbeddedTexture> mTextures;
    Noesis::Ptr<TextureProvider> mFallback;
};

NS_WARNING_POP

}

#endif
