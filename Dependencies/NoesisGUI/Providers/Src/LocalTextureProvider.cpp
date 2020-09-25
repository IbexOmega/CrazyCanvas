////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/LocalTextureProvider.h>
#include <NsCore/Ptr.h>
#include <NsCore/StringUtils.h>
#include <NsGui/Stream.h>


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
LocalTextureProvider::LocalTextureProvider(const char* rootPath)
{
    StrCopy(mRootPath, sizeof(mRootPath), rootPath);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Stream> LocalTextureProvider::OpenStream(const char* filename) const
{
    char uri[512];

    if (StrIsNullOrEmpty(mRootPath))
    {
        StrCopy(uri, sizeof(uri), filename);
    }
    else
    {
        StrCopy(uri, sizeof(uri), mRootPath);
        StrAppend(uri, sizeof(uri), "/");
        StrAppend(uri, sizeof(uri), filename);
    }

    return OpenFileStream(uri);
}
