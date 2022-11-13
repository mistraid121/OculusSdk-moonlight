/************************************************************************************

Filename    :   GlProgram.cpp
Content     :   Shader program compilation.
Created     :   October 11, 2013
Authors     :   John Carmack

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#include "GlProgram.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "VrApi_Types.h"
#include "Misc/Log.h"

#include "OVR_Std.h"

#include <string>

namespace OVRFW
{
static bool UseMultiview = false;

// All GlPrograms implicitly get the VertexHeader
static const char * VertexHeader =
R"=====(
#ifndef DISABLE_MULTIVIEW
 #define DISABLE_MULTIVIEW 0
#endif
#define NUM_VIEWS 2
#if __VERSION__ < 300
  #define in attribute
  #define out varying
#else
  #define attribute in
  #define varying out
#endif
#if defined( GL_OVR_multiview2 ) && ! DISABLE_MULTIVIEW && __VERSION__ >= 300
  #extension GL_OVR_multiview2 : require
  layout(num_views=NUM_VIEWS) in;
  #define VIEW_ID gl_ViewID_OVR
#else
  uniform lowp int ViewID;
  #define VIEW_ID ViewID
#endif

uniform highp mat4 ModelMatrix;
#if __VERSION__ >= 300
// Use a ubo in v300 path to workaround corruption issue on Adreno 420+v300
// when uniform array of matrices used.
uniform SceneMatrices
{
	highp mat4 ViewMatrix[NUM_VIEWS];
	highp mat4 ProjectionMatrix[NUM_VIEWS];
} sm;
// Use a function macro for TransformVertex to workaround an issue on exynos8890+Android-M driver:
// gl_ViewID_OVR outside of the vertex main block causes VIEW_ID to be 0 for every view.
// NOTE: Driver fix has landed with Android-N.
//highp vec4 TransformVertex( highp vec4 oPos )
//{
//	highp vec4 hPos = sm.ProjectionMatrix[VIEW_ID] * ( sm.ViewMatrix[VIEW_ID] * ( ModelMatrix * oPos ) );
//	return hPos;
//}
#define TransformVertex(localPos) (sm.ProjectionMatrix[VIEW_ID] * ( sm.ViewMatrix[VIEW_ID] * ( ModelMatrix * localPos )))
#else
// Still support v100 for image_external as v300 support is lacking in Mali-T760/Android-L drivers.
uniform highp mat4 ViewMatrix[NUM_VIEWS];
uniform highp mat4 ProjectionMatrix[NUM_VIEWS];
highp vec4 TransformVertex( highp vec4 oPos )
{
	highp vec4 hPos = ProjectionMatrix[VIEW_ID] * ( ViewMatrix[VIEW_ID] * ( ModelMatrix * oPos ) );
	return hPos;
}
#endif
)====="
;

// All GlPrograms implicitly get the FragmentHeader
static const char * FragmentHeader =
R"=====(
#if __VERSION__ < 300
	#define in varying
#else
	#define varying in
	#define gl_FragColor fragColor
	out mediump vec4 fragColor;
	#define texture2D texture
	#define textureCube texture
#endif
)====="
;

// ----DEPRECATED_GLPROGRAM

GlProgram BuildProgram( const char * vertexDirectives, const char * vertexSrc, const char * fragmentDirectives, const char * fragmentSrc, const int programVersion )
{
	return GlProgram::Build( vertexDirectives, vertexSrc, fragmentDirectives, fragmentSrc, nullptr, 0, programVersion, true /* abort on error */, true /* use deprecated interface */ );
}

GlProgram BuildProgram( const char * vertexSrc, const char * fragmentSrc, const int programVersion )
{
	return BuildProgram( nullptr, vertexSrc, nullptr, fragmentSrc, programVersion );
}

GlProgram BuildProgramNoMultiview( const char * vertexDirectives, const char * vertexSrc,
		const char * fragmentDirectives, const char * fragmentSrc )
{
	const bool useMultiview = UseMultiview;
	GlProgram::SetUseMultiview( false );
	GlProgram prog = BuildProgram( vertexDirectives, vertexSrc, fragmentDirectives, fragmentSrc );
	GlProgram::SetUseMultiview( useMultiview );
	return prog;
}

void DeleteProgram( GlProgram & prog )
{
	GlProgram::Free( prog );
}
// ----DEPRECATED_GLPROGRAM

static const char * FindShaderVersionEnd( const char * src )
{
	if ( src == nullptr || OVR::OVR_strncmp( src, "#version ", 9 ) != 0 )
	{
		return src;
	}
	while ( *src != 0 && *src != '\n' )
	{
		src++;
	}
	if ( *src == '\n' )
	{
		src++;
	}
	return src;
}

static GLuint CompileShader( GLenum shaderType, const char * directives, const char * src, GLint programVersion )
{
	const char * postVersion = FindShaderVersionEnd( src );
	if ( postVersion != src )
	{
		ALOGW( "GlProgram: #version in source is not supported. Specify at program build time." );
	}

	std::string srcString;

	// Valid versions for GL ES:
	// #version 100      -- ES 2.0
	// #version 300 es	 -- ES 3.0
	// #version 310 es	 -- ES 3.1
	const char * versionModifier = ( programVersion > 100 ) ? "es" : "";

	srcString = std::string("#version ") + std::to_string(programVersion) + std::string(" ") + versionModifier + std::string("\n");

	if ( directives != nullptr )
	{
		srcString += directives;
	}

	srcString += std::string("#define DISABLE_MULTIVIEW ") + std::to_string( UseMultiview  ? 0 : 1 ) + std::string("\n");

	if ( shaderType == GL_VERTEX_SHADER )
	{
		srcString += VertexHeader;
	}
	else if ( shaderType == GL_FRAGMENT_SHADER )
	{
		srcString += FragmentHeader;
	}

	srcString += postVersion ;

	src = srcString.c_str();

	GLuint shader = glCreateShader( shaderType );

	const int numSources = 1;
	const char * srcs[1];
	srcs[0] = src;

	glShaderSource( shader, numSources, srcs, 0 );
	glCompileShader( shader );

	GLint r;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &r );
	if ( r == GL_FALSE )
	{
		ALOGW( "Compiling %s shader: ****** failed ******\n", shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment" );
		GLchar msg[1024];
		const char * sp = src;
		int charCount = 0;
		int line = 0;
		do
		{
			if ( *sp != '\n' )
			{
				msg[charCount++] = *sp;
				msg[charCount] = 0;
			}
			if ( *sp == 0 || *sp == '\n' || charCount == 1023 )
			{
				charCount = 0;
				line++;
				ALOGW( "%03d  %s", line, msg );
				msg[0] = 0;
				if ( *sp != '\n' )
				{
					line--;
				}
			}
			sp++;
		} while( *sp != 0 );
		if ( charCount != 0 )
		{
			line++;
			ALOGW( "%03d  %s", line, msg );
		}
		glGetShaderInfoLog( shader, sizeof( msg ), 0, msg );
		ALOGW( "%s\n", msg );
		glDeleteShader( shader );
		return 0;
	}
	return shader;
}

GlProgram GlProgram::Build( const char * vertexSrc,
							const char * fragmentSrc,
							const ovrProgramParm * parms, const int numParms,
							const int requestedProgramVersion,
							bool abortOnError, bool useDeprecatedInterface )
{
	return Build( nullptr, vertexSrc, nullptr, fragmentSrc, parms, numParms,
					requestedProgramVersion, abortOnError, useDeprecatedInterface );
}

GlProgram GlProgram::Build( const char * vertexDirectives, const char * vertexSrc,
							const char * fragmentDirectives, const char * fragmentSrc,
							const ovrProgramParm * parms, const int numParms,
							const int requestedProgramVersion,
							bool abortOnError, bool useDeprecatedInterface )
{
	GlProgram p;
	p.UseDeprecatedInterface = useDeprecatedInterface;

	//--------------------------
	// Compile and Create the Program
	//--------------------------

	int programVersion = requestedProgramVersion;
	if ( programVersion < GLSL_PROGRAM_VERSION )
	{
		ALOGW( "GlProgram: Program GLSL version requested %d, but does not meet required minimum %d",
			requestedProgramVersion, GLSL_PROGRAM_VERSION );
		programVersion = GLSL_PROGRAM_VERSION;
	}
	// ----IMAGE_EXTERNAL_WORKAROUND
	// GL_OES_EGL_image_external extension support issues:
	// -- Both Adreno and Mali drivers do not maintain support for base GL_OES_EGL_image_external
	//    when compiled against v300. Instead, GL_OES_EGL_image_external_essl3 is required.
	// -- Mali drivers for T760 + Android-L currently list both extensions as unsupported under v300
	//    and fail to recognize 'samplerExternalOES' on compile.
	// P0003: Warning: Extension 'GL_OES_EGL_image_external' not supported
	// P0003: Warning: Extension 'GL_OES_EGL_image_external_essl3' not supported
	// L0001: Expected token '{', found 'identifier' (samplerExternalOES)
	//
	// Currently, it appears that drivers which fully support multiview also support
	// GL_OES_EGL_image_external_essl3 with v300. In the case where multiview is not
	// fully supported, we force the shader version to v100 in order to maintain support
	// for image_external with the Mali T760+Android-L drivers.
	if ( !UseMultiview && (
			( fragmentDirectives != nullptr && strstr( fragmentDirectives, "GL_OES_EGL_image_external" ) != nullptr ) ||
			( strstr( fragmentSrc, "GL_OES_EGL_image_external" ) != nullptr ) ) )
	{
		ALOG( "GlProgram: Program GLSL version v100 due to GL_OES_EGL_image_external use." );
		programVersion = 100;
	}
	// ----IMAGE_EXTERNAL_WORKAROUND

	p.VertexShader = CompileShader( GL_VERTEX_SHADER, vertexDirectives, vertexSrc, programVersion );
	if ( p.VertexShader == 0 )
	{
		Free( p );
		if ( abortOnError )
		{
			ALOGE_FAIL( "Failed to compile vertex shader" );
		}
		return GlProgram();
	}

	p.FragmentShader = CompileShader( GL_FRAGMENT_SHADER, fragmentDirectives, fragmentSrc, programVersion );
	if ( p.FragmentShader == 0 )
	{
		Free( p );
		if ( abortOnError )
		{
			ALOGE_FAIL( "Failed to compile fragment shader" );
		}
		return GlProgram();
	}

	p.Program = glCreateProgram();
	glAttachShader( p.Program, p.VertexShader );
	glAttachShader( p.Program, p.FragmentShader );

	//--------------------------
	// Set attributes before linking
	//--------------------------

	glBindAttribLocation( p.Program, VERTEX_ATTRIBUTE_LOCATION_POSITION,		"Position" );
	glBindAttribLocation( p.Program, VERTEX_ATTRIBUTE_LOCATION_NORMAL,			"Normal" );
	glBindAttribLocation( p.Program, VERTEX_ATTRIBUTE_LOCATION_TANGENT,			"Tangent" );
	glBindAttribLocation( p.Program, VERTEX_ATTRIBUTE_LOCATION_BINORMAL,		"Binormal" );
	glBindAttribLocation( p.Program, VERTEX_ATTRIBUTE_LOCATION_COLOR,			"VertexColor" );
	glBindAttribLocation( p.Program, VERTEX_ATTRIBUTE_LOCATION_UV0,				"TexCoord" );
	glBindAttribLocation( p.Program, VERTEX_ATTRIBUTE_LOCATION_UV1,				"TexCoord1" );
	glBindAttribLocation( p.Program, VERTEX_ATTRIBUTE_LOCATION_JOINT_INDICES,	"JointIndices" );
	glBindAttribLocation( p.Program, VERTEX_ATTRIBUTE_LOCATION_JOINT_WEIGHTS,	"JointWeights" );
	glBindAttribLocation( p.Program, VERTEX_ATTRIBUTE_LOCATION_FONT_PARMS,		"FontParms" );

	//--------------------------
	// Link Program
	//--------------------------

	glLinkProgram( p.Program );

	GLint linkStatus;
	glGetProgramiv( p.Program, GL_LINK_STATUS, &linkStatus );
	if ( linkStatus == GL_FALSE )
	{
		GLchar msg[1024];
		glGetProgramInfoLog( p.Program, sizeof( msg ), 0, msg );
		Free( p );
		ALOG( "Linking program failed: %s\n", msg );
		if ( abortOnError )
		{
			ALOGE_FAIL( "Failed to link program" );
		}
		return GlProgram();
	}

	//--------------------------
	// Determine Uniform Parm Location and Binding.
	//--------------------------

	p.numTextureBindings = 0;
	p.numUniformBufferBindings = 0;

	// Globally-defined system level uniforms.
	{
		p.ViewID.Type	  = ovrProgramParmType::INT;
		p.ViewID.Location = glGetUniformLocation( p.Program, "ViewID" );
		p.ViewID.Binding  = p.ViewID.Location;

		p.SceneMatrices.Type = ovrProgramParmType::BUFFER_UNIFORM;
		p.SceneMatrices.Location = glGetUniformBlockIndex( p.Program, "SceneMatrices" );
		if ( p.SceneMatrices.Location >= 0 )	// this won't be present for v100 shaders.
		{
			p.SceneMatrices.Binding = p.numUniformBufferBindings++;
			glUniformBlockBinding( p.Program, p.SceneMatrices.Location, p.SceneMatrices.Binding );
		}

		p.ModelMatrix.Type	 = ovrProgramParmType::FLOAT_MATRIX4;
		p.ModelMatrix.Location = glGetUniformLocation( p.Program, "ModelMatrix" );
		p.ModelMatrix.Binding  = p.ModelMatrix.Location;

		// ----IMAGE_EXTERNAL_WORKAROUND
		p.ViewMatrix.Type	 = ovrProgramParmType::FLOAT_MATRIX4;
		p.ViewMatrix.Location = glGetUniformLocation( p.Program, "ViewMatrix" );
		p.ViewMatrix.Binding  = p.ViewMatrix.Location;

		p.ProjectionMatrix.Type	 = ovrProgramParmType::FLOAT_MATRIX4;
		p.ProjectionMatrix.Location = glGetUniformLocation( p.Program, "ProjectionMatrix" );
		p.ProjectionMatrix.Binding  = p.ProjectionMatrix.Location;
		// ----IMAGE_EXTERNAL_WORKAROUND
	}

	// ----DEPRECATED_GLPROGRAM
	// old materialDef members - these will go away soon
	p.uMvp				= glGetUniformLocation( p.Program, "Mvpm" );
	p.uModel			= glGetUniformLocation( p.Program, "Modelm" );
	p.uColor			= glGetUniformLocation( p.Program, "UniformColor" );
	p.uFadeDirection	= glGetUniformLocation( p.Program, "UniformFadeDirection" );
	p.uTexm				= glGetUniformLocation( p.Program, "Texm" );
	p.uColorTableOffset = glGetUniformLocation( p.Program, "ColorTableOffset" );
	p.uClipUVs			= glGetUniformLocation( p.Program, "ClipUVs" );
	p.uJoints			= glGetUniformBlockIndex( p.Program, "JointMatrices" );
	p.uUVOffset			= glGetUniformLocation( p.Program, "UniformUVOffset" );
	if ( p.uJoints != -1 )
	{
		p.uJointsBinding = p.numUniformBufferBindings++;
		glUniformBlockBinding( p.Program, p.uJoints, p.uJointsBinding );
	}
	// ^^ old materialDef members - these should go away eventually - ideally soon
	// ----DEPRECATED_GLPROGRAM

	glUseProgram( p.Program );

	for ( int i = 0; i < numParms; ++i )
	{
		assert( parms[i].Type != ovrProgramParmType::MAX );
		p.Uniforms[i].Type = parms[i].Type;
		if ( parms[i].Type == ovrProgramParmType::TEXTURE_SAMPLED )
		{
			p.Uniforms[i].Location = static_cast<int16_t>( glGetUniformLocation( p.Program, parms[i].Name ) );
			p.Uniforms[i].Binding = p.numTextureBindings++;
			glUniform1i( p.Uniforms[i].Location, p.Uniforms[i].Binding );
		}
		else if ( parms[i].Type == ovrProgramParmType::BUFFER_UNIFORM )
		{
			p.Uniforms[i].Location = glGetUniformBlockIndex( p.Program, parms[i].Name );
			p.Uniforms[i].Binding = p.numUniformBufferBindings++;
			glUniformBlockBinding( p.Program, p.Uniforms[i].Location, p.Uniforms[i].Binding );
		}
		else
		{
			p.Uniforms[i].Location = static_cast<int16_t>( glGetUniformLocation( p.Program, parms[i].Name ) );
			p.Uniforms[i].Binding = p.Uniforms[i].Location;
		}

#ifdef OVR_BUILD_DEBUG
		if ( p.Uniforms[i].Location < 0 || p.Uniforms[i].Binding < 0 )
		{
			ALOG( "GlProgram::Build. Invalid shader parm: %s", parms[i].Name );
		}
#endif

		assert( p.Uniforms[i].Location >= 0 && p.Uniforms[i].Binding >= 0 );
	}

	// implicit texture and image_external bindings
	for ( int i = 0; i < ovrUniform::MAX_UNIFORMS; i++ )
	{
		char name[32];
		sprintf( name, "Texture%i", i );
		const GLint uTex = glGetUniformLocation( p.Program, name );
		if ( uTex != -1 )
		{
			glUniform1i( uTex, i );
		}
	}

	glUseProgram( 0 );

	return p;
}

void GlProgram::Free( GlProgram & prog )
{
	glUseProgram( 0 );
	if ( prog.Program != 0 )
	{
		glDeleteProgram( prog.Program );
	}
	if ( prog.VertexShader != 0 )
	{
		glDeleteShader( prog.VertexShader );
	}
	if ( prog.FragmentShader != 0 )
	{
		glDeleteShader( prog.FragmentShader );
	}
	prog.Program = 0;
	prog.VertexShader = 0;
	prog.FragmentShader = 0;
}

void GlProgram::SetUseMultiview( const bool useMultiview_ )
{
	UseMultiview = useMultiview_;
}

}	// namespace OVRFW
