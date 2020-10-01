////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsApp/EmbeddedXamlProvider.h>
#include <NsCore/StringUtils.h>
#include <NsGui/MemoryStream.h>


using namespace Noesis;
using namespace NoesisApp;


////////////////////////////////////////////////////////////////////////////////////////////////////
EmbeddedXamlProvider::EmbeddedXamlProvider(ArrayRef<EmbeddedXaml> xamls, XamlProvider* fallback):
    mFallback(fallback)
{
    mXamls.Assign(xamls.Begin(), xamls.End());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
Ptr<Stream> EmbeddedXamlProvider::LoadXaml(const char* uri)
{
    for (const auto& xaml: mXamls)
    {
        if (StrEquals(uri, xaml.name))
        {
            return *new MemoryStream(xaml.data.Data(), xaml.data.Size());
        }
    }

    return mFallback ? mFallback->LoadXaml(uri) : nullptr;
}
