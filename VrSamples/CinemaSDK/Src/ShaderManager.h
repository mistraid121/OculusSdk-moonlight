/************************************************************************************

Filename    :   ShaderManager.h
Content     :	Allocates and builds shader programs.
Created     :	7/3/2014
Authors     :   Jim Dosé and John Carmack

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( ShaderManager_h )
#define ShaderManager_h

#include "Model/ModelFile.h"


namespace OculusCinema {

class CinemaApp;

enum sceneProgram_t
{
	SCENE_PROGRAM_BLACK,
	SCENE_PROGRAM_STATIC_ONLY,
	SCENE_PROGRAM_STATIC_DYNAMIC,
	SCENE_PROGRAM_DYNAMIC_ONLY,
	SCENE_PROGRAM_ADDITIVE,
	SCENE_PROGRAM_MAX
};

class ShaderManager
{
public:
							ShaderManager( CinemaApp &cinema );

	void					OneTimeInit( const char * launchIntent );
	void					OneTimeShutdown();

	CinemaApp &				Cinema;

	// Render the external image texture to a conventional texture to allow
	// mipmap generation.
    OVRFW::GlProgram				CopyMovieProgram;

    OVRFW::GlProgram				ProgVertexColor;
    OVRFW::GlProgram				ProgSingleTexture;
    OVRFW::GlProgram				ProgLightMapped;
    OVRFW::GlProgram				ProgReflectionMapped;
    OVRFW::GlProgram				ProgSkinnedVertexColor;
    OVRFW::GlProgram				ProgSkinnedSingleTexture;
    OVRFW::GlProgram				ProgSkinnedLightMapped;
    OVRFW::GlProgram				ProgSkinnedReflectionMapped;

    OVRFW::GlProgram				ScenePrograms[ SCENE_PROGRAM_MAX ];

    OVRFW::ModelGlPrograms 		DynamicPrograms;
    OVRFW::ModelGlPrograms 		DefaultPrograms;

private:
	ShaderManager &			operator=( const ShaderManager & );
};

} // namespace OculusCinema

#endif // ShaderManager_h
