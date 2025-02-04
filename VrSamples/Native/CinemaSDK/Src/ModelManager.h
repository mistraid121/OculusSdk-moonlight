/************************************************************************************

Filename    :   ModelManager.h
Content     :
Created     :	7/3/2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( ModelManager_h )
#define ModelManager_h

#include "ModelFile.h"
#include <vector>
#include <string>

using namespace OVR;

namespace OculusCinema {

class CinemaApp;

class SceneDef
{
public:
						SceneDef() : 
							SceneModel( NULL ),
							Filename(),
							UseFreeScreen( false ),
							UseSeats( false ),
							UseDynamicProgram( false ),
							Loaded( false ),
							UseVRScreen( false ) { }


	ModelFile *			SceneModel;
	std::string			Filename;
	GlTexture			IconTexture;
	bool				UseFreeScreen;
	bool 				UseSeats;
	bool 				UseDynamicProgram;
	bool				Loaded;
	bool                UseVRScreen;

};

class ModelManager
{
public:
						ModelManager( CinemaApp &cinema );
						~ModelManager();

	void				OneTimeInit( const char * launchIntent );
	void				OneTimeShutdown();

	int					GetTheaterCount() const { return static_cast< int >( Theaters.size() ); }
	const SceneDef & 	GetTheater( int index ) const;

public:
	CinemaApp &			Cinema;

	std::vector<SceneDef *>	Theaters;
	SceneDef *			BoxOffice;
	SceneDef *			VoidScene;
	SceneDef *          VRScene;

	std::string			LaunchIntent;

	ModelFile *			DefaultSceneModel;

private:
	ModelManager &		operator=( const ModelManager & );

	void 				LoadModels();
	void 				ScanDirectoryForScenes( const std::string & directoryString, bool useDynamicProgram, std::vector<SceneDef *> &scenes ) const;
	SceneDef *			LoadScene( const char * filename, bool useDynamicProgram, bool loadFromApplicationPackage ) const;
};

} // namespace OculusCinema

#endif // ModelManager_h
