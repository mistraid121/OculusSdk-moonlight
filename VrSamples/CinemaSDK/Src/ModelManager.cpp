/************************************************************************************

Filename    :   ModelManager.cpp
Content     :
Created     :	7/3/2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "ModelManager.h"
#include "CinemaApp.h"
#include "PackageFiles.h"
#include "OVR_BinaryFile2.h"

#include <algorithm>

#if defined( OVR_OS_ANDROID )
#include <dirent.h>
#endif

using namespace OVRFW;

namespace OculusCinema {

static const char * TheatersDirectory = "Oculus/Cinema/Theaters";

//=======================================================================================

ModelManager::ModelManager( CinemaApp &cinema ) :
	Cinema( cinema ),
	Theaters(),
	BoxOffice( NULL ),
	VoidScene( NULL ),
	LaunchIntent(),
	DefaultSceneModel( NULL )

{
}

ModelManager::~ModelManager()
{
}

void ModelManager::OneTimeInit( const char * launchIntent )
{
	OVR_LOG( "ModelManager::OneTimeInit" );
	const double start = GetTimeInSeconds();
	LaunchIntent = launchIntent;

	DefaultSceneModel = new ModelFile( "default" );

	LoadModels();

	OVR_LOG( "ModelManager::OneTimeInit: %i theaters loaded, %3.1f seconds", static_cast< int >( Theaters.size() ),  GetTimeInSeconds() - start );
}

void ModelManager::OneTimeShutdown()
{
	OVR_LOG( "ModelManager::OneTimeShutdown" );

	// Free GL resources

	for( OVR::UPInt i = 0; i < static_cast< OVR::UPInt >( Theaters.size() ); i++ )
	{
		delete Theaters[ i ];
	}
}

void ModelManager::LoadModels()
{
	OVR_LOG( "ModelManager::LoadModels" );
	const double start =  GetTimeInSeconds();

	BoxOffice = LoadScene( "assets/scenes/BoxOffice.ovrscene", false, true );
	BoxOffice->UseSeats = false;

	if ( LaunchIntent.length() > 0 )
	{
		Theaters.push_back( LoadScene( LaunchIntent.c_str(), true, false ) );
	}
	else
	{
		// we want our theaters to show up first
		Theaters.push_back( LoadScene( "assets/scenes/home_theater.ovrscene", true, true ) );

		// create void scene
		VoidScene = new SceneDef();
		VoidScene->SceneModel = new ModelFile( "Void" );
		VoidScene->UseSeats = false;
		VoidScene->UseDynamicProgram = false;
		VoidScene->UseFreeScreen = true;
		//        VoidScene->UseVRScreen = true;


		int width = 0, height = 0;
		VoidScene->IconTexture = LoadTextureFromApplicationPackage( "assets/VoidTheater.png",
				TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), width, height );

		BuildTextureMipmaps( GlTexture( VoidScene->IconTexture, width, height ) );
		MakeTextureTrilinear( GlTexture( VoidScene->IconTexture, width, height ) );
		MakeTextureClamped( GlTexture( VoidScene->IconTexture, width, height ) );

		Theaters.push_back( VoidScene );

		// create void scene
		VRScene = new SceneDef();
		VRScene->SceneModel = new ModelFile( "VR" );
		VRScene->UseSeats = false;
		VRScene->UseDynamicProgram = false;
		VRScene->UseFreeScreen = true;
		VRScene->UseVRScreen = true;

		VRScene->IconTexture = LoadTextureFromApplicationPackage( "assets/VRTheater.png",
																  TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), width, height );

		BuildTextureMipmaps( VRScene->IconTexture );
		MakeTextureTrilinear( VRScene->IconTexture );
		MakeTextureClamped( VRScene->IconTexture );

		//TODO activar el modo vr
		//Theaters.push_back( VRScene );


		// load all scenes on startup, so there isn't a delay when switching theaters
		ScanDirectoryForScenes( Cinema.ExternalRetailDir( TheatersDirectory ), true, Theaters );
		ScanDirectoryForScenes( Cinema.RetailDir( TheatersDirectory ), true, Theaters );
		ScanDirectoryForScenes( Cinema.SDCardDir( TheatersDirectory ), true, Theaters );
	}

	OVR_LOG( "ModelManager::LoadModels: %i theaters loaded, %3.1f seconds", static_cast< int >( Theaters.size() ),  GetTimeInSeconds() - start );
}

void ModelManager::ScanDirectoryForScenes( const std::string & directoryString, bool useDynamicProgram, std::vector<SceneDef *> &scenes ) const
{
	const char * directory = directoryString.c_str();

	DIR * dir = opendir( directory );
	if ( dir != NULL )
	{
		struct dirent * entry;
		while( ( entry = readdir( dir ) ) != NULL ) {
			std::string filename = entry->d_name;
			std::string ext = filename.substr(filename.rfind("."));
			std::transform( ext.begin(), ext.end(), ext.begin(), ::tolower);
			if ( ( ext == ".ovrscene" ) )
			{
				std::string fullpath = directory;
				fullpath += "/";
				fullpath += filename;
				SceneDef *def = LoadScene( fullpath.c_str(), useDynamicProgram, false );
				scenes.push_back( def );
			}
		}

		closedir( dir );
	}
}

SceneDef * ModelManager::LoadScene( const char *sceneFilename, bool useDynamicProgram, bool loadFromApplicationPackage ) const
{
	std::string filename;

	if ( loadFromApplicationPackage && !ovr_PackageFileExists( sceneFilename ) )
	{
		OVR_LOG( "Scene %s not found in application package.  Checking sdcard.", sceneFilename );
		loadFromApplicationPackage = false;
	}

	if ( loadFromApplicationPackage )
	{
		filename = sceneFilename;
	}
	else if ( ( sceneFilename != NULL ) && ( *sceneFilename == '/' ) ) 	// intent will have full path for scene file, so check for /
	{
		filename = sceneFilename;
	}
	else if ( Cinema.FileExists( Cinema.ExternalRetailDir( sceneFilename ) ) )
	{
		filename = Cinema.ExternalRetailDir( sceneFilename );
	}
	else if ( Cinema.FileExists( Cinema.RetailDir( sceneFilename ) ) )
	{
		filename = Cinema.RetailDir( sceneFilename );
	}
	else
	{
		filename = Cinema.SDCardDir( sceneFilename );
	}

	OVR_LOG( "Adding scene: %s, %s", filename.c_str(), sceneFilename );

	SceneDef *def = new SceneDef();
	def->Filename = sceneFilename;
	def->UseSeats = true;
	def->UseDynamicProgram = useDynamicProgram;

	MaterialParms materialParms;
	materialParms.UseSrgbTextureFormats = Cinema.GetUseSrgb();
	// Improve the texture quality with anisotropic filtering.
	materialParms.EnableDiffuseAniso = true;
	// The emissive texture is used as a separate lighting texture and should not be LOD clamped.
	materialParms.EnableEmissiveLodClamp = false;

	ModelGlPrograms glPrograms = ( useDynamicProgram ) ? Cinema.ShaderMgr.DynamicPrograms : Cinema.ShaderMgr.DefaultPrograms;

	std::string iconFilename = filename.substr(0, filename.rfind(".") + 1 ) + "png" ;

	int textureWidth = 0, textureHeight = 0;

	if ( loadFromApplicationPackage )
	{
		def->SceneModel = LoadModelFileFromApplicationPackage( filename.c_str(), glPrograms, materialParms );
		def->IconTexture = LoadTextureFromApplicationPackage( iconFilename.c_str(), TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), textureWidth, textureHeight );
	}
	else
	{
		def->SceneModel = LoadModelFile( filename.c_str(), glPrograms, materialParms );
		def->IconTexture = LoadTextureFromBuffer( iconFilename.c_str(), MemBufferFile( iconFilename.c_str() ),
				TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), textureWidth, textureHeight );
	}

	if ( def->SceneModel == nullptr )
	{
		OVR_WARN( "Could not load scenemodel %s", filename.c_str() );
		def->SceneModel = new ModelFile( "Default Scene Model" );
	}

	if ( def->IconTexture != 0 )
	{
		OVR_LOG( "Loaded external icon for theater: %s", iconFilename.c_str() );
	}
	else
	{
		const ModelTexture * iconTexture = def->SceneModel->FindNamedTexture( "icon" );
		if ( iconTexture != NULL )
		{
			def->IconTexture = iconTexture->texid;
		}
		else
		{
			OVR_LOG( "No icon in scene.  Loading default." );

			int	width = 0, height = 0;
			def->IconTexture = LoadTextureFromApplicationPackage( "assets/noimage.png",
				TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), width, height );
		}
	}

	BuildTextureMipmaps( def->IconTexture );
	MakeTextureTrilinear( def->IconTexture );
	MakeTextureClamped( def->IconTexture );

	def->UseFreeScreen = false;

	return def;
}

const SceneDef & ModelManager::GetTheater( int index ) const
{
	if ( index < static_cast< int >( Theaters.size() ) )
	{
		return *Theaters[ index ];
	}

	// default to the Void Scene
	return *VoidScene;
}

} // namespace OculusCinema
