/************************************************************************************

Filename    :   UITexture.cpp
Content     :
Created     :	1/8/2015
Authors     :   Jim Dose

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#include "UI/UITexture.h"
#include "App.h"
#include "PackageFiles.h"
#include "OVR_BinaryFile2.h"

namespace OVR
{
UITexture::UITexture() : Width( 0 ), Height( 0 ), Texture( 0 ), FreeTextureOfDestruct( false ) {}

UITexture::~UITexture()
{
	Free();
}

void UITexture::Free()
{
	if ( FreeTextureOfDestruct )
	{
		glDeleteTextures( 1, &Texture );
	}
	Texture				  = 0;
	Width				  = 0;
	Height				  = 0;
	FreeTextureOfDestruct = false;
}

// Depricated.  Use LoadTextureFromUri.
void UITexture::LoadTextureFromApplicationPackage( const char * assetPath )
{
#if defined( OVR_OS_ANDROID )
	Free();
	Texture =
		OVR::LoadTextureFromApplicationPackage( assetPath, TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), Width, Height );
	FreeTextureOfDestruct = true;
#else
	auto mbf = MemBufferFile( assetPath );
	LoadTextureFromBuffer( assetPath, mbf );
#endif
}

void UITexture::LoadTextureFromUri( ovrFileSys & fileSys, const char * uri )
{
	Free();
	Texture = OVR::LoadTextureFromUri( fileSys, uri, TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), Width, Height );
	FreeTextureOfDestruct = true;
}

void UITexture::LoadTextureFromBuffer( const char * fileName, const std::vector<uint8_t> & buffer )
{
	Free();
	Texture = OVR::LoadTextureFromBuffer( fileName, buffer, TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), Width, Height );
	FreeTextureOfDestruct = true;
}

void UITexture::LoadTextureFromMemory( const uint8_t * data, const int width, const int height )
{
	OVR_LOG( "UITexture::LoadTextureFromMemory" );
	Free();
	Width				  = width;
	Height				  = height;
	Texture				  = OVR::LoadRGBATextureFromMemory( data, width, height, true );
	FreeTextureOfDestruct = true;
}

void UITexture::SetTexture( const GLuint texture, const int width, const int height, const bool freeTextureOnDestruct )
{
	Free();
	FreeTextureOfDestruct = freeTextureOnDestruct;
	Texture				  = texture;
	Width				  = width;
	Height				  = height;
}

}	// namespace OVR
