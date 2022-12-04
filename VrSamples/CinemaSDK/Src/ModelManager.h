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

#include "Model/ModelFile.h"
#include <vector>
#include <string>

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
        Loaded( false )
        { }


    OVRFW::ModelFile *			SceneModel;
	std::string			Filename;
    OVRFW::GlTexture			IconTexture;
	bool				UseFreeScreen;
	bool 				UseSeats;
	bool 				UseDynamicProgram;
	bool				Loaded;
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
	SceneDef *			VoidScene;

private:
	ModelManager &		operator=( const ModelManager & );

	void 				LoadModels(std::string launchIntent);
	void 				ScanDirectoryForScenes( const std::string & directoryString, bool useDynamicProgram, std::vector<SceneDef *> &scenes ) const;
	SceneDef *			LoadScene( const char * filename, bool useDynamicProgram, bool loadFromApplicationPackage ) const;
};

} // namespace OculusCinema

#endif // ModelManager_h
