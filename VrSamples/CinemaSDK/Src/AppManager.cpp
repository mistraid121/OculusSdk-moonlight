/************************************************************************************

Filename    :   AppManager.cpp
Content     :
Created     :	9/10/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "OVR_JSON.h"
#include "OVR_BinaryFile2.h"

#include "AppManager.h"
#include "CinemaApp.h"
#include "PackageFiles.h"
#include "Native.h"


#if defined( OVR_OS_ANDROID )
#include <dirent.h>
#endif

#include <sys/stat.h>
#include <errno.h>

using namespace OVRFW;

namespace OculusCinema {


static void ReplaceExtension(std::string & s, const std::string & newExtension)
{
    // find the last .
    size_t lastDot = s.rfind('.');
    if (lastDot != std::string::npos)
    {
        s.replace( s.begin() + lastDot, s.end(), newExtension );
    }
    else
    {
        s += newExtension;
    }
}
//=======================================================================================

AppManager::AppManager(CinemaApp &cinema) :
        Apps(),
        Cinema(cinema),
        DefaultPoster(0) {
}

AppManager::~AppManager() {
}

void AppManager::OneTimeInit(const char *launchIntent) {
    OVR_LOG("AppManager::OneTimeInit");

    OVR_UNUSED(launchIntent);

    const double start = GetTimeInSeconds();

    int width, height;
    DefaultPoster = LoadTextureFromApplicationPackage(
            "assets/default_poster.png",
            TextureFlags_t(TEXTUREFLAG_NO_DEFAULT), width, height);
    OVR_LOG(" Default gluint: %i", DefaultPoster);

    BuildTextureMipmaps(GlTexture(DefaultPoster, width, height));
    MakeTextureTrilinear(GlTexture(DefaultPoster, width, height));
    MakeTextureClamped(GlTexture(DefaultPoster, width, height));

    OVR_LOG("AppManager::OneTimeInit: %3.1f seconds",GetTimeInSeconds() - start);
}

void AppManager::OneTimeShutdown() {
    OVR_LOG("AppManager::OneTimeShutdown");
}

void AppManager::AddApp(const std::string &name, const std::string &posterFileName, int id, bool isRunning)
{
    OVR_LOG("App %s with id %i added!", name.c_str(), id);
    AppDef *anApp = NULL;
    bool isNew = false;
    for (OVR::UPInt i = 0; i < Apps.size(); i++) {
        if (OVR::OVR_stricmp(Apps[i]->Name.c_str(),name.c_str()) == 0)
            anApp = Apps[i];
    }
    if (anApp == NULL) {
        anApp = new AppDef();
        Apps.push_back(anApp);
        isNew = true;
    }

    anApp->Name = name;
    anApp->Id = id;
    anApp->PosterFileName = posterFileName;
    anApp->isRunning = isRunning;

    if (isNew) ReadMetaData(anApp);
    if (anApp->Poster == 0) LoadPoster(anApp);
}

void AppManager::RemoveApp(int id) {
    for (OVR::UPInt i = 0; i < Apps.size(); i++)
        if (Apps[i]->Id == id) {
            Apps.erase( Apps.cbegin() + i );
            i--;
        }
}

void AppManager::ReadMetaData( AppDef *anApp )
{
    std::string filename = anApp->Name;
    ReplaceExtension( filename, ".app.txt" );

    const char* error = NULL;

    if ( !Cinema.FileExists( filename.c_str() ) )
    {
        return;
    }

    if ( auto metadata = OVR::JSON::Load( filename.c_str(), &error ) )
    {

       // metadata->Release();

        OVR_LOG( "Loaded metadata: %s", filename.c_str() );
    }
    else
    {
        OVR_LOG( "Error loading metadata for %s: %s", filename.c_str(), ( error == NULL ) ? "NULL" : error );
    }
}

void AppManager::LoadPoster( AppDef *anApp ) {
    std::string posterFilename = anApp->PosterFileName;
    ReplaceExtension( posterFilename, ".png" );

    anApp->Poster = LoadTextureFromBuffer(posterFilename.c_str(),
                                          MemBufferFile(posterFilename.c_str()),
                                          TextureFlags_t(TEXTUREFLAG_NO_DEFAULT),
                                          anApp->PosterWidth, anApp->PosterHeight);
    OVR_LOG("Poster loaded: %s %i %i %i", posterFilename.c_str(), anApp->Poster,
        anApp->PosterWidth, anApp->PosterHeight);
    //if (anApp->Poster == 0) {
    anApp->Poster = DefaultPoster;
    /*} else {

        BuildTextureMipmaps(GlTexture(anApp->Poster, anApp->PosterWidth, anApp->PosterHeight));
        MakeTextureTrilinear(GlTexture(anApp->Poster, anApp->PosterWidth, anApp->PosterHeight));
        MakeTextureClamped(GlTexture(anApp->Poster, anApp->PosterWidth, anApp->PosterHeight));
    }*/
}

void AppManager::LoadPosters()
{
    for(OVR::UPInt i=0; i < Apps.size(); i++)
    {
        /*if( Apps[i]->Poster == 0 || Apps[i]->Poster == DefaultPoster )
        {
            LoadPoster(Apps[i]);
        }*/
        LoadPoster(Apps[i]);
    }
}


} // namespace OculusCinema
