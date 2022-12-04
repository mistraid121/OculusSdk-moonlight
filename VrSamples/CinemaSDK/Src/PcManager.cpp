/************************************************************************************

Filename    :   PcManager.cpp
Content     :
Created     :	9/10/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "OVR_BinaryFile2.h"
#include "OVR_JSON.h"
#include "OVR_Std.h"

#include "PcManager.h"
#include "CinemaApp.h"
#include "PackageFiles.h"

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

PcManager::PcManager( CinemaApp &cinema ) :
	updated(false),Cinema(cinema),Pcs()
{
}

PcManager::~PcManager()
{
}

void PcManager::OneTimeInit( const char * launchIntent )
{
	OVR_LOG( "PcManager::OneTimeInit" );

	OVR_UNUSED( launchIntent );

	const double start =  GetTimeInSeconds();
	int width, height;

    PcPosterPaired = LoadTextureFromApplicationPackage(
            "assets/generic_paired_poster.png",
            TextureFlags_t(TEXTUREFLAG_NO_DEFAULT), width, height);
	BuildTextureMipmaps( GlTexture( PcPosterPaired, width, height ) );
	MakeTextureTrilinear( GlTexture( PcPosterPaired, width, height ) );
	MakeTextureClamped( GlTexture( PcPosterPaired, width, height ) );


    PcPosterUnpaired = LoadTextureFromApplicationPackage(
            "assets/generic_unpaired_poster.png",
            TextureFlags_t(TEXTUREFLAG_NO_DEFAULT), width, height);
	BuildTextureMipmaps( GlTexture( PcPosterUnpaired, width, height ) );
	MakeTextureTrilinear( GlTexture( PcPosterUnpaired, width, height ) );
	MakeTextureClamped( GlTexture( PcPosterUnpaired, width, height ) );

	PcPosterUnknown = LoadTextureFromApplicationPackage(
            "assets/generic_unknown_poster.png",
            TextureFlags_t(TEXTUREFLAG_NO_DEFAULT), width, height);
	BuildTextureMipmaps( GlTexture( PcPosterUnknown, width, height ) );
	MakeTextureTrilinear( GlTexture( PcPosterUnknown, width, height ) );
	MakeTextureClamped( GlTexture( PcPosterUnknown, width, height ) );

    PcPosterWTF = LoadTextureFromApplicationPackage(
            "assets/generic_wtf_poster.png",
            TextureFlags_t(TEXTUREFLAG_NO_DEFAULT), width, height);
    BuildTextureMipmaps( GlTexture( PcPosterWTF, width, height ) );
    MakeTextureTrilinear( GlTexture( PcPosterWTF, width, height ) );
    MakeTextureClamped( GlTexture( PcPosterWTF, width, height ) );

    OVR_LOG("PcManager::OneTimeInit: %3.1f seconds",GetTimeInSeconds() - start);

}

void PcManager::OneTimeShutdown()
{
	OVR_LOG( "PcManager::OneTimeShutdown" );
}

void PcManager::AddPc(const char *name, const char *uuid, Native::PairState pairState, Native::Reachability reachability,const char *binding, const bool isRunning) {
	PcDef *pc = NULL;
	bool isNew = false;

	for (OVR::UPInt i = 0; i < Pcs.size(); i++) {
		if (OVR::OVR_stricmp( Pcs[i]->Name.c_str(), name ) == 0)
			pc = Pcs[i];
	}
	if (pc == NULL) {
		pc = new PcDef();
		Pcs.push_back(pc);
		isNew = true;
	}

	pc->Name = name;
	pc->UUID = uuid;
	pc->Binding = binding;
	pc->isRunning = isRunning;
	pc->isRemote = reachability == Native::REMOTE;

	if (isNew) {
		ReadMetaData(pc);
	}
	pc->Poster = PcPosterWTF;
	switch(pairState) {
		case Native::NOT_PAIRED:	pc->Poster = PcPosterUnpaired; break;
		case Native::PAIRED:		pc->Poster = PcPosterPaired; break;
		case Native::PIN_WRONG:		pc->Poster = PcPosterWTF; break;
		case Native::FAILED:
		default: 					pc->Poster = PcPosterUnknown; break;
	}
	updated = true;
}

void PcManager::RemovePc(const std::string &name) {
	for (OVR::UPInt i = 0; i < Pcs.size(); i++) {
		if (OVR::OVR_stricmp( Pcs[i]->Name.c_str(), name.c_str() ) == 0)
			continue;
		Pcs.erase( Pcs.cbegin() + i );
		return;
	}
}

void PcManager::ReadMetaData( PcDef *pc )
{
	std::string filename = pc->Name;
	ReplaceExtension( filename, ".txt" );

	const char* error = NULL;

	if ( !Cinema.FileExists( filename.c_str() ) )
	{
		return;
	}

	if (auto metadata = OVR::JSON::Load(filename.c_str(), &error)) {

		//metadata->Release();

		OVR_LOG("Loaded metadata: %s", filename.c_str());
	} else {
		OVR_LOG("Error loading metadata for %s: %s", filename.c_str(),
				(error == NULL) ? "NULL" : error);
	}
}


} // namespace OculusCinema
