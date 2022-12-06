/************************************************************************************

Filename    :   ShaderManager.cpp
Content     :	Allocates and builds shader programs.
Created     :	7/3/2014
Authors     :   Jim Dosé and John Carmack

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "ShaderManager.h"
#include "CinemaApp.h"

using namespace OVRFW;

namespace OculusCinema {

//=======================================================================================

static const char * ImageExternalDirectives =
#if defined( OVR_OS_ANDROID )
	"#extension GL_OES_EGL_image_external : enable\n"
	"#extension GL_OES_EGL_image_external_essl3 : enable\n";
#else
	"";
#endif

static const char * copyMovieVertexShaderSrc =
	"attribute vec4 Position;\n"
	"attribute vec2 TexCoord;\n"
	"varying  highp vec2 oTexCoord;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = Position;\n"
	"   oTexCoord = vec2( TexCoord.x, 1.0 - TexCoord.y );\n"	// need to flip Y
	"}\n";

static const char * copyMovieFragmentShaderSource =
#if defined( OVR_OS_ANDROID )
	"uniform samplerExternalOES Texture0;\n"
#else
	"uniform sampler2D Texture0;\n"
#endif
	"uniform sampler2D Texture1;\n"								// edge vignette
	"varying highp vec2 oTexCoord;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor = texture2D( Texture0, oTexCoord ) *  texture2D( Texture1, oTexCoord );\n"
	"}\n";

static const char * SceneStaticVertexShaderSrc =
	"uniform lowp vec4 UniformColor;\n"
	"attribute vec4 Position;\n"
	"attribute vec2 TexCoord;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying lowp vec4 oColor;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = TransformVertex( Position );\n"
	"   oTexCoord = TexCoord;\n"
	"   oColor = UniformColor;\n"
	"}\n";

static const char * SceneDynamicVertexShaderSrc =
	"uniform sampler2D Texture2;\n"
	"uniform lowp vec4 UniformColor;\n"
	"attribute vec4 Position;\n"
	"attribute vec2 TexCoord;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying lowp vec4 oColor;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = TransformVertex( Position );\n"
	"   oTexCoord = TexCoord;\n"
	"   oColor = textureLod(Texture2, vec2( 0.0, 0.0), 16.0 );\n"	// bottom mip of screen texture
	"   oColor.xyz += vec3( 0.05, 0.05, 0.05 );\n"
	"	oColor.w = UniformColor.w;\n"
	"}\n";

static const char * SceneBlackFragmentShaderSrc =
	"void main()\n"
	"{\n"
	"	gl_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );\n"
	"}\n";
/*
static const char * SceneStaticFragmentShaderSrc =
	"uniform sampler2D Texture0;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying lowp vec4 oColor;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor.xyz = oColor.w * texture2D(Texture0, oTexCoord).xyz;\n"
	"	gl_FragColor.w = 1.0;\n"
	"}\n";
*/
static const char * SceneStaticAndDynamicFragmentShaderSrc =
	"uniform sampler2D Texture0;\n"
	"uniform sampler2D Texture1;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying lowp vec4 oColor;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor.xyz = oColor.w * texture2D(Texture0, oTexCoord).xyz + (1.0 - oColor.w) * oColor.xyz * texture2D(Texture1, oTexCoord).xyz;\n"
	"	gl_FragColor.w = 1.0;\n"
	"}\n";
/*
static const char * SceneDynamicFragmentShaderSrc =
	"uniform sampler2D Texture0;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying lowp vec4 oColor;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor.xyz = (1.0 - oColor.w) * oColor.xyz * texture2D(Texture0, oTexCoord).xyz;\n"
	"	gl_FragColor.w = 1.0;\n"
	"}\n";
*/
static const char * SceneAdditiveFragmentShaderSrc =
	"uniform sampler2D Texture0;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying lowp vec4 oColor;\n"
	"void main()\n"
	"{\n"
	"	gl_FragColor.xyz = (1.0 - oColor.w) * texture2D(Texture0, oTexCoord).xyz;\n"
	"	gl_FragColor.w = 1.0;\n"
	"}\n";

//=======================================================================================

ShaderManager::ShaderManager( CinemaApp &cinema ) :
	Cinema( cinema )
{
}

void ShaderManager::OneTimeInit( const char * launchIntent )
{
	OVR_LOG( "ShaderManager::OneTimeInit" );

	OVR_UNUSED( launchIntent );

	const double start = GetTimeInSeconds();

	/// Init Shaders
	{
		/// Disable multi-view for this ...
		OVRFW::GlProgram::MultiViewScope(false);

		static OVRFW::ovrProgramParm uniformParms[] = {
				/// Vertex
				/// Fragment
				{"Texture0", OVRFW::ovrProgramParmType::TEXTURE_SAMPLED},
				{"Texture1", OVRFW::ovrProgramParmType::TEXTURE_SAMPLED},
		};
		CopyMovieProgram = OVRFW::GlProgram::Build(
				nullptr,
				copyMovieVertexShaderSrc,
				ImageExternalDirectives,
				copyMovieFragmentShaderSource,
				uniformParms,
				sizeof(uniformParms) / sizeof(OVRFW::ovrProgramParm));
	}

	{
		static OVRFW::ovrProgramParm uniformParms[] = {
				/// Vertex
				{"UniformColor", OVRFW::ovrProgramParmType::FLOAT_VECTOR4},
				/// Fragment
		};
		const int uniformCount = sizeof(uniformParms) / sizeof(OVRFW::ovrProgramParm);
		ScenePrograms[SCENE_PROGRAM_BLACK] = OVRFW::GlProgram::Build(
				SceneStaticVertexShaderSrc, SceneBlackFragmentShaderSrc, uniformParms, uniformCount);
	}
	//ScenePrograms[SCENE_PROGRAM_STATIC_ONLY]	= BuildProgram( SceneStaticVertexShaderSrc, SceneStaticFragmentShaderSrc );
	{
		static OVRFW::ovrProgramParm uniformParms[] = {
				/// Vertex
				{"UniformColor", OVRFW::ovrProgramParmType::FLOAT_VECTOR4},
				{"Texture2", OVRFW::ovrProgramParmType::TEXTURE_SAMPLED},
				/// Fragment
				{"Texture0", OVRFW::ovrProgramParmType::TEXTURE_SAMPLED},
				{"Texture1", OVRFW::ovrProgramParmType::TEXTURE_SAMPLED},
		};
		const int uniformCount = sizeof(uniformParms) / sizeof(OVRFW::ovrProgramParm);
		ScenePrograms[SCENE_PROGRAM_STATIC_DYNAMIC] = OVRFW::GlProgram::Build(
				SceneDynamicVertexShaderSrc,
				SceneStaticAndDynamicFragmentShaderSrc,
				uniformParms,
				uniformCount);
	}
	//ScenePrograms[SCENE_PROGRAM_DYNAMIC_ONLY]	= BuildProgram( SceneDynamicVertexShaderSrc, SceneDynamicFragmentShaderSrc );
	{
		static OVRFW::ovrProgramParm uniformParms[] = {
				/// Vertex
				{"UniformColor", OVRFW::ovrProgramParmType::FLOAT_VECTOR4},
				/// Fragment
				{"Texture0", OVRFW::ovrProgramParmType::TEXTURE_SAMPLED},
		};
		const int uniformCount = sizeof(uniformParms) / sizeof(OVRFW::ovrProgramParm);
		ScenePrograms[SCENE_PROGRAM_ADDITIVE] = OVRFW::GlProgram::Build(
				SceneStaticVertexShaderSrc, SceneAdditiveFragmentShaderSrc, uniformParms, uniformCount);
	}

	// NOTE: make sure to load with SCENE_PROGRAM_STATIC_DYNAMIC because the textures are initially not swapped
	DynamicPrograms = ModelGlPrograms( &ScenePrograms[ SCENE_PROGRAM_STATIC_DYNAMIC ] );

	//ProgVertexColor				= BuildProgram( VertexColorVertexShaderSrc, VertexColorFragmentShaderSrc );
	{
		OVRFW::ovrProgramParm uniformParms[] = {
				/// Vertex
				/// Fragment
				{"Texture0", OVRFW::ovrProgramParmType::TEXTURE_SAMPLED},
		};
		const int uniformCount = sizeof(uniformParms) / sizeof(OVRFW::ovrProgramParm);
		ProgSingleTexture = OVRFW::GlProgram::Build(
				OVRFW::SingleTextureVertexShaderSrc,
				OVRFW::SingleTextureFragmentShaderSrc,
				uniformParms,
				uniformCount);
	}
	/*
	ProgLightMapped				= BuildProgram( LightMappedVertexShaderSrc, LightMappedFragmentShaderSrc );
	ProgReflectionMapped		= BuildProgram( ReflectionMappedVertexShaderSrc, ReflectionMappedFragmentShaderSrc );
	ProgSkinnedVertexColor		= BuildProgram( VertexColorSkinned1VertexShaderSrc, VertexColorFragmentShaderSrc );
	ProgSkinnedSingleTexture	= BuildProgram( SingleTextureSkinned1VertexShaderSrc, SingleTextureFragmentShaderSrc );
	ProgSkinnedLightMapped		= BuildProgram( LightMappedSkinned1VertexShaderSrc, LightMappedFragmentShaderSrc );
	ProgSkinnedReflectionMapped	= BuildProgram( ReflectionMappedSkinned1VertexShaderSrc, ReflectionMappedFragmentShaderSrc );
*/
	DefaultPrograms.ProgVertexColor				= & ProgVertexColor;
	DefaultPrograms.ProgSingleTexture			= & ProgSingleTexture;
	DefaultPrograms.ProgLightMapped				= & ProgLightMapped;
	DefaultPrograms.ProgReflectionMapped		= & ProgReflectionMapped;
	DefaultPrograms.ProgSkinnedVertexColor		= & ProgSkinnedVertexColor;
	DefaultPrograms.ProgSkinnedSingleTexture	= & ProgSkinnedSingleTexture;
	DefaultPrograms.ProgSkinnedLightMapped		= & ProgSkinnedLightMapped;
	DefaultPrograms.ProgSkinnedReflectionMapped	= & ProgSkinnedReflectionMapped;

	OVR_LOG( "ShaderManager::OneTimeInit: %3.1f seconds", GetTimeInSeconds() - start );
}

void ShaderManager::OneTimeShutdown()
{
	OVR_LOG( "ShaderManager::OneTimeShutdown" );

	OVRFW::GlProgram::Free( CopyMovieProgram );

	OVRFW::GlProgram::Free( ScenePrograms[SCENE_PROGRAM_BLACK] );
	OVRFW::GlProgram::Free( ScenePrograms[SCENE_PROGRAM_STATIC_ONLY] );
	OVRFW::GlProgram::Free( ScenePrograms[SCENE_PROGRAM_STATIC_DYNAMIC] );
	OVRFW::GlProgram::Free( ScenePrograms[SCENE_PROGRAM_DYNAMIC_ONLY] );
	OVRFW::GlProgram::Free( ScenePrograms[SCENE_PROGRAM_ADDITIVE] );

	//TODO: Review DynamicPrograms

	OVRFW::GlProgram::Free( ProgVertexColor );
	OVRFW::GlProgram::Free( ProgSingleTexture );
	OVRFW::GlProgram::Free( ProgLightMapped );
	OVRFW::GlProgram::Free( ProgReflectionMapped );
	OVRFW::GlProgram::Free( ProgSkinnedVertexColor );
	OVRFW::GlProgram::Free( ProgSkinnedSingleTexture	);
	OVRFW::GlProgram::Free( ProgSkinnedLightMapped );
	OVRFW::GlProgram::Free( ProgSkinnedReflectionMapped );
}

} // namespace OculusCinema
