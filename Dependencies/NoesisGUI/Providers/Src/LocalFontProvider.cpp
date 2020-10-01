////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/LocalFontProvider.h>
#include <NsCore/Find.h>
#include <NsGui/Stream.h>


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
LocalFontProvider::LocalFontProvider(const char* rootPath)
{
    StrCopy(mRootPath, sizeof(mRootPath), rootPath);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LocalFontProvider::ScanFolder(const char* folder)
{
    char uri[512] = "";

    if (!StrIsNullOrEmpty(mRootPath))
    {
        StrCopy(uri, sizeof(uri), mRootPath);
        StrAppend(uri, sizeof(uri), "/");
    }

    StrAppend(uri, sizeof(uri), folder);

    ScanFolder(uri, folder, ".ttf");
    ScanFolder(uri, folder, ".otf");
    ScanFolder(uri, folder, ".ttc");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Stream> LocalFontProvider::OpenFont(const char* folder, const char* filename) const
{
    char uri[512] = "";

    if (!StrIsNullOrEmpty(mRootPath))
    {
        StrCopy(uri, sizeof(uri), mRootPath);
        StrAppend(uri, sizeof(uri), "/");
    }

    StrAppend(uri, sizeof(uri), folder);
    StrAppend(uri, sizeof(uri), "/");
    StrAppend(uri, sizeof(uri), filename);

    return OpenFileStream(uri);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void LocalFontProvider::ScanFolder(const char* path, const char* folder, const char* ext)
{
    FindData findData;

    if (FindFirst(path, ext, findData))
    {
        do
        {
            RegisterFont(folder, findData.filename);
        }
        while (FindNext(findData));

        FindClose(findData);
    }
}
