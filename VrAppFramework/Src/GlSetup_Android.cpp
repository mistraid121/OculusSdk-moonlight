/************************************************************************************

Filename    :   GlSetup.cpp
Content     :   GL Setup
Created     :   August 24, 2013
Authors     :   John Carmack

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

************************************************************************************/

#include "GlSetup.h"
#include "OVR_Types.h"

#if defined( OVR_OS_ANDROID )

#include "OVR_LogUtils.h"

// TODO: remove when new glext is available
#if !defined( EGL_OPENGL_ES3_BIT_KHR )
#define EGL_OPENGL_ES3_BIT_KHR		0x0040
#endif

namespace OVR {

static void LogStringWords( const char * allExtensions )
{
	const char * start = allExtensions;
	while( 1 )
	{
		const char * end = strstr( start, " " );
		if ( end == NULL )
		{
			break;
		}
		unsigned int nameLen = (unsigned int)(end - start);
		if ( nameLen > 256 )
		{
			nameLen = 256;
		}
		char * word = new char[nameLen+1];
		memcpy( word, start, nameLen );
		word[nameLen] = '\0';
		OVR_LOG( "%s", word );
		delete[] word;

		start = end + 1;
	}
}

#if defined( OVR_BUILD_DEBUG )

static void DumpEglConfigs( const EGLDisplay display )
{
	static const int MAX_CONFIGS = 1024;
	EGLConfig configs[MAX_CONFIGS];
	EGLint numConfigs = 0;

	if ( EGL_FALSE == eglGetConfigs( display,
			configs, MAX_CONFIGS, &numConfigs ) )
	{
		OVR_WARN( "eglGetConfigs() failed" );
		return;
	}

	OVR_LOG( "EGL configs:" );
	OVR_LOG( "  Config R G B A DP S M W P REND SURF" );
	for ( int i = 0; i < numConfigs; i++ )
	{
		EGLint	red = 0;
		eglGetConfigAttrib( display, configs[i], EGL_RED_SIZE, &red );
		EGLint	green = 0;
		eglGetConfigAttrib( display, configs[i], EGL_GREEN_SIZE, &green );
		EGLint	blue = 0;
		eglGetConfigAttrib( display, configs[i], EGL_BLUE_SIZE, &blue );
		EGLint	alpha = 0;
		eglGetConfigAttrib( display, configs[i], EGL_ALPHA_SIZE, &alpha );
		EGLint	depth = 0;
		eglGetConfigAttrib( display, configs[i], EGL_DEPTH_SIZE, &depth );
		EGLint	stencil = 0;
		eglGetConfigAttrib( display, configs[i], EGL_STENCIL_SIZE, &stencil );
		EGLint	multisamples = 0;
		eglGetConfigAttrib( display, configs[i], EGL_SAMPLES, &multisamples );

		// EGL_SURFACE_TYPE is a bit field
		EGLint	surface = 0;
		eglGetConfigAttrib( display, configs[i], EGL_SURFACE_TYPE , &surface );
		EGLint window = ( surface & EGL_WINDOW_BIT ) != 0;
		EGLint pbuffer = ( surface & EGL_PBUFFER_BIT ) != 0;

		// EGL_RENDERABLE_TYPE is a bit field
		EGLint	renderable = 0;
		eglGetConfigAttrib( display, configs[i], EGL_RENDERABLE_TYPE, &renderable );

		OVR_LOG( "%p %i %i %i %i %2i %i %i %i %i 0x%02x 0x%02x", configs[i], red, green, blue, alpha,
				depth, stencil, multisamples, window, pbuffer, renderable, surface );
	}
}
#endif

// Returns NULL if no config is found.
static EGLConfig ChooseColorConfig( const EGLDisplay display, const int redBits,
		const int greenBits, const int blueBits, const int depthBits, const int samples, const bool pbuffer )
{
#if defined( OVR_BUILD_DEBUG )
	DumpEglConfigs( display );
#endif

	// We do NOT want to use eglChooseConfig, because the Android EGL code pushes in
	// multisample flags behind our back if the user has selected the "force 4x MSAA"
	// option in settings, and that is completely wasted for our warp target.
	static const int MAX_CONFIGS = 1024;
	EGLConfig 	configs[MAX_CONFIGS];
	EGLint  	numConfigs = 0;

	if ( EGL_FALSE == eglGetConfigs( display,
			configs, MAX_CONFIGS, &numConfigs ) ) {
		OVR_WARN( "eglGetConfigs() failed" );
		return NULL;
	}
	OVR_LOG( "eglGetConfigs() = %i configs", numConfigs );

	// We don't want a depth/stencil buffer
	const EGLint configAttribs[] = {
			EGL_ALPHA_SIZE, 	redBits == 8 ? 8 : 0, // need alpha for the multi-pass timewarp compositor
			EGL_BLUE_SIZE,  	blueBits,
			EGL_GREEN_SIZE, 	greenBits,
			EGL_RED_SIZE,   	redBits,
			EGL_DEPTH_SIZE,   	depthBits,
//			EGL_STENCIL_SIZE,  	0,
			EGL_SAMPLES,		samples,
			EGL_NONE
	};

	for ( int i = 0; i < numConfigs; i++ )
	{
		EGLint	value = 0;

		// EGL_RENDERABLE_TYPE is a bit field
		eglGetConfigAttrib( display, configs[i], EGL_RENDERABLE_TYPE, &value );

		if ( ( value & EGL_OPENGL_ES3_BIT_KHR ) != EGL_OPENGL_ES3_BIT_KHR )
		{
			continue;
		}

		// For our purposes, the pbuffer config also needs to be compatible with
		// normal window rendering so it can share textures with the window context.
		// I am unsure if it would be portable to always request EGL_PBUFFER_BIT, so
		// I only do it on request.
		eglGetConfigAttrib( display, configs[i], EGL_SURFACE_TYPE , &value );
		const int surfs = EGL_WINDOW_BIT | ( pbuffer ? EGL_PBUFFER_BIT : 0 );
		if ( ( value & surfs ) != surfs )
		{
			continue;
		}

		int	j = 0;
		for ( ; configAttribs[j] != EGL_NONE; j += 2 )
		{
			EGLint	value = 0;
			eglGetConfigAttrib( display, configs[i], configAttribs[j], &value );
			if ( value != configAttribs[j+1] )
			{
				break;
			}
		}
		if ( configAttribs[j] == EGL_NONE )
		{
			OVR_LOG( "Found a renderable config: %p", configs[i] );
			return configs[i];
		}
	}
	return NULL;
}

glSetup_t GL_Setup( const EGLContext shareContext,
		const int requestedGlEsVersion,
		const int redBits, const int greenBits, const int blueBits,
		const int depthBits, const int multisamples, const GLuint contextPriority )
{
	OVR_LOG( "GL_Setup: requestGlEsVersion(%d), redBits(%d), greenBits(%d), blueBits(%d), depthBits(%d), multisamples(%d), contextPriority(%d)",
			requestedGlEsVersion, redBits, greenBits, blueBits, depthBits, multisamples, contextPriority );

	glSetup_t egl = {};

	// Get the built in display
	// TODO: check for external HDMI displays
	egl.display = eglGetDisplay( EGL_DEFAULT_DISPLAY );

	// Initialize EGL
	EGLint majorVersion;
	EGLint minorVersion;
	eglInitialize( egl.display, &majorVersion, &minorVersion );
	OVR_LOG( "eglInitialize gives majorVersion %i, minorVersion %i", majorVersion, minorVersion);

	const char * eglVendorString = eglQueryString( egl.display, EGL_VENDOR );
	OVR_LOG( "EGL_VENDOR: %s", eglVendorString );
	const char * eglClientApisString = eglQueryString( egl.display, EGL_CLIENT_APIS );
	OVR_LOG( "EGL_CLIENT_APIS: %s", eglClientApisString );
	const char * eglVersionString = eglQueryString( egl.display, EGL_VERSION );
	OVR_LOG( "EGL_VERSION: %s", eglVersionString );
	const char * eglExtensionString = eglQueryString( egl.display, EGL_EXTENSIONS );
	OVR_LOG( "EGL_EXTENSIONS:" );
	LogStringWords( eglExtensionString );

	// We do NOT want to use eglChooseConfig, because the Android EGL code pushes in
	// multisample flags behind our back if the user has selected the "force 4x MSAA"
	// option in developer settings, and that is completely wasted for our warp target.
	egl.config = ChooseColorConfig( egl.display, redBits, greenBits, blueBits, depthBits, multisamples, true /* pBuffer compatible */ );
	if ( egl.config == 0 )
	{
		OVR_FAIL( "No acceptable EGL color configs." );
		return egl;
	}

	// The EGLContext is created with the EGLConfig
	// Try to get an OpenGL ES 3.0 context first, which is required to do
	// MSAA to framebuffer objects on Adreno.
	for ( int version = requestedGlEsVersion; version >= 2; version-- )
	{
		OVR_LOG( "Trying for a EGL_CONTEXT_CLIENT_VERSION %i context shared with %p:",
				version, shareContext );
		// We want the application context to be lower priority than the TimeWarp context.
		EGLint contextAttribs[] = {
				EGL_CONTEXT_CLIENT_VERSION, version,
				EGL_NONE, EGL_NONE,
				EGL_NONE };

		// Don't set EGL_CONTEXT_PRIORITY_LEVEL_IMG at all if set to EGL_CONTEXT_PRIORITY_MEDIUM_IMG,
		// It is the caller's responsibility to use that if the driver doesn't support it.
		if ( contextPriority != EGL_CONTEXT_PRIORITY_MEDIUM_IMG )
		{
			contextAttribs[2] = EGL_CONTEXT_PRIORITY_LEVEL_IMG;
			contextAttribs[3] = contextPriority;
		}

		egl.context = eglCreateContext( egl.display, egl.config, shareContext, contextAttribs );
		if ( egl.context != EGL_NO_CONTEXT )
		{
			OVR_LOG( "Succeeded." );
			egl.glEsVersion = version;

			EGLint configIDReadback;
			if ( !eglQueryContext( egl.display, egl.context, EGL_CONFIG_ID, &configIDReadback ) )
			{
			     OVR_WARN("eglQueryContext EGL_CONFIG_ID failed" );
			}
			EGLConfig configCheck = EglConfigForConfigID( egl.display, configIDReadback );

			OVR_LOG( "Created context with config %p, query returned ID %i = config %p",
					egl.config, configIDReadback, configCheck );
			break;
		}
	}
	if ( egl.context == EGL_NO_CONTEXT )
	{
		OVR_WARN( "eglCreateContext failed: %s", GL_GetErrorString() );
		return egl;
	}

	if ( contextPriority != EGL_CONTEXT_PRIORITY_MEDIUM_IMG )
	{
		// See what context priority we actually got
		EGLint actualPriorityLevel;
		eglQueryContext( egl.display, egl.context, EGL_CONTEXT_PRIORITY_LEVEL_IMG, &actualPriorityLevel );
		switch ( actualPriorityLevel )
		{
			case EGL_CONTEXT_PRIORITY_HIGH_IMG:		OVR_LOG( "Context is EGL_CONTEXT_PRIORITY_HIGH_IMG" ); break;
			case EGL_CONTEXT_PRIORITY_MEDIUM_IMG:	OVR_LOG( "Context is EGL_CONTEXT_PRIORITY_MEDIUM_IMG" ); break;
			case EGL_CONTEXT_PRIORITY_LOW_IMG:		OVR_LOG( "Context is EGL_CONTEXT_PRIORITY_LOW_IMG" ); break;
			default:								OVR_LOG( "Context has unknown priority level" ); break;
		}
	}

	// Because EGL_KHR_surfaceless_context is not widespread (Only on Tegra as of
	// September 2013), we need to create a tiny pbuffer surface to make the context
	// current.
	//
	// It is necessary to use a config with the same characteristics that the
	// context was created with, plus the pbuffer flag, or we will get an
	// EGL_BAD_MATCH error on the eglMakeCurrent() call.
	//
	// This is necessary to support 565 framebuffers, which may be important
	// for higher refresh rate displays.
	const EGLint attrib_list[] =
	{
		EGL_WIDTH, 16,
		EGL_HEIGHT, 16,
		EGL_NONE
	};
	egl.pbufferSurface = eglCreatePbufferSurface( egl.display, egl.config, attrib_list );
	if ( egl.pbufferSurface == EGL_NO_SURFACE )
	{
		OVR_WARN( "eglCreatePbufferSurface failed: %s", GL_GetErrorString() );
		eglDestroyContext( egl.display, egl.context );
		egl.context = EGL_NO_CONTEXT;
		return egl;
	}

	if ( eglMakeCurrent( egl.display, egl.pbufferSurface, egl.pbufferSurface, egl.context ) == EGL_FALSE )
	{
		OVR_WARN( "eglMakeCurrent pbuffer failed: %s", GL_GetErrorString() );
		eglDestroySurface( egl.display, egl.pbufferSurface );
		eglDestroyContext( egl.display, egl.context );
		egl.context = EGL_NO_CONTEXT;
		return egl;
	}

	const char * glVendorString = (const char *) glGetString(GL_VENDOR);
	OVR_LOG( "GL_VENDOR: %s", glVendorString );
	const char * glRendererString = (const char *) glGetString(GL_RENDERER);
	OVR_LOG( "GL_RENDERER: %s", glRendererString );
	const char * glVersionString = (const char *) glGetString(GL_VERSION);
	OVR_LOG( "GL_VERSION: %s", glVersionString );
	const char * glSlVersionString = (const char *) glGetString(
			GL_SHADING_LANGUAGE_VERSION );
	OVR_LOG( "GL_SHADING_LANGUAGE_VERSION: %s", glSlVersionString );

	return egl;
}

void GL_Shutdown( glSetup_t & glSetup )
{
	if ( eglMakeCurrent( glSetup.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) == EGL_FALSE )
	{
		OVR_FAIL( "eglMakeCurrent: failed: %s", GL_GetErrorString() );
	}
	if ( eglDestroyContext( glSetup.display, glSetup.context ) == EGL_FALSE )
	{
		OVR_FAIL( "eglDestroyContext: failed: %s", GL_GetErrorString() );
	}
	if ( eglDestroySurface( glSetup.display, glSetup.pbufferSurface ) == EGL_FALSE )
	{
		OVR_FAIL( "eglDestroySurface: failed: %s", GL_GetErrorString() );
	}
	if ( eglTerminate( glSetup.display ) == EGL_FALSE )
	{
		OVR_FAIL( "eglTerminate(): failed: %s", GL_GetErrorString() );
	}

	glSetup.glEsVersion = 0;
	glSetup.display = 0;
	glSetup.pbufferSurface = 0;
	glSetup.config = 0;
	glSetup.context = 0;
}

bool GL_ProcessEvents( int32_t & exitCode )
{
	exitCode = 0;
	return false;
}

}	// namespace OVR

#endif	// OVR_OS_ANDROID
