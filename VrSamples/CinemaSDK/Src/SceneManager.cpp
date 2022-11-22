/************************************************************************************

Filename    :   SceneManager.cpp
Content     :	Handles rendering of current scene and movie.
Created     :   September 3, 2013
Authors     :	Jim Dose, based on a fork of VrVideo.cpp from VrVideo by John Carmack.

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "CinemaApp.h"
#include "Native.h"
#include "SceneManager.h"
#include "GUI/GuiSys.h"

#include <algorithm>


// Returns true if head equals check plus zero or more characters.
inline bool MatchesHead( const char * head, const char * check )
{
	const int l = static_cast<int>( OVR::OVR_strlen( head ) );
	return 0 == OVR::OVR_strncmp( head, check, l );
}

using namespace OVRFW;
using OVR::Matrix4f;
using OVR::Vector2f;
using OVR::Vector3f;
using OVR::Posef;
using OVR::Quatf;
using OVR::Bounds3f;

namespace OculusCinema
{

SceneManager::SceneManager( CinemaApp &cinema ) :
        Cinema( cinema ),
        StaticLighting(),
        MovieTexture( NULL ),
        MovieTextureTimestamp( 0 ),
        FreeScreenActive( false ),
        FreeScreenScale( 1.0f ),
        FreeScreenDistance( 1.5f ),
        FreeScreenPose(),
        CurrentMovieWidth( 0 ),
        CurrentMovieHeight( 480 ),
        MovieTextureWidth( 0 ),
        MovieTextureHeight( 0 ),
        FrameUpdateNeeded( false ),
        ClearGhostsFrames( 0 ),
        UnitSquare(),
        ScreenColor(),
        CurrentMipMappedMovieTexture( 0 ),
        MipMappedMovieTextureSwapChain( NULL ),
        MipMappedMovieTextureSwapChainLength( 0 ),
        MipMappedMovieFBOs( NULL ),
        BufferSize(),
        ScreenVignetteTexture( 0 ),
        SceneProgramIndex( SCENE_PROGRAM_DYNAMIC_ONLY ),
        Scene(),
        SceneScreenSurface( NULL ),
        SceneSeatPositions(),
        SceneSeatCount( 0 ),
        SeatPosition( 0 ),
        SceneScreenBounds(),
        VoidedScene( false ),
        FixVoidedScene( false )

{
}

static GlGeometry BuildFadedScreenMask( const float xFraction, const float yFraction )
{
    const float posx[] = { -1.001f, -1.0f + xFraction * 0.25f, -1.0f + xFraction, 1.0f - xFraction, 1.0f - xFraction * 0.25f, 1.001f };
    const float posy[] = { -1.001f, -1.0f + yFraction * 0.25f, -1.0f + yFraction, 1.0f - yFraction, 1.0f - yFraction * 0.25f, 1.001f };

    const int vertexCount = 6 * 6;

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uv0.resize( vertexCount );
    attribs.color.resize( vertexCount );

    for ( int y = 0; y < 6; y++ )
    {
        for ( int x = 0; x < 6; x++ )
        {
            const int index = y * 6 + x;
            attribs.position[index].x = posx[x];
            attribs.position[index].y = posy[y];
            attribs.position[index].z = 0.0f;
            attribs.uv0[index].x = 0.0f;
            attribs.uv0[index].y = 0.0f;
            // the outer edges will have 0 color
            const float c = ( y <= 1 || y >= 4 || x <= 1 || x >= 4 ) ? 0.0f : 1.0f;
            for ( int i = 0; i < 3; i++ )
            {
                attribs.color[index][i] = c;
            }
            attribs.color[index][3] = 1.0f;	// solid alpha
        }
    }

    std::vector< TriangleIndex > indices;
    indices.resize( 25 * 6 );

    // Should we flip the triangulation on the corners?
    int index = 0;
    for ( TriangleIndex x = 0; x < 5; x++ )
    {
        for ( TriangleIndex y = 0; y < 5; y++ )
        {
            indices[index + 0] = y * 6 + x;
            indices[index + 1] = y * 6 + x + 1;
            indices[index + 2] = (y + 1) * 6 + x;
            indices[index + 3] = (y + 1) * 6 + x;
            indices[index + 4] = y * 6 + x + 1;
            indices[index + 5] = (y + 1) * 6 + x + 1;
            index += 6;
        }
    }

    return GlGeometry( attribs, indices );
}

static const char * overlayScreenFadeMaskVertexShaderSrc =
        "attribute vec4 VertexColor;\n"
                "attribute vec4 Position;\n"
                "varying  lowp vec4 oColor;\n"
                "void main()\n"
                "{\n"
                "   gl_Position = TransformVertex( Position );\n"
                "   oColor = vec4( 1.0, 1.0, 1.0, 1.0 - VertexColor.x );\n"
                "}\n";

static const char * overlayScreenFadeMaskFragmentShaderSrc =
        "varying lowp vec4	oColor;\n"
                "void main()\n"
                "{\n"
                "	gl_FragColor = oColor;\n"
                "}\n";

void SceneManager::OneTimeInit( const char * launchIntent )
{
	OVR_LOG( "SceneManager::OneTimeInit" );

    OVR_UNUSED( launchIntent );

    const double start = GetTimeInSeconds();


	/// Init Rendering
	SurfaceRender.Init();

    UnitSquare = BuildTesselatedQuad( 1, 1 );

    ScreenVignetteTexture = BuildScreenVignetteTexture( 1 );

    FadedScreenMaskSquareDef.graphicsCommand.Program = GlProgram::Build(
            overlayScreenFadeMaskVertexShaderSrc,
            overlayScreenFadeMaskFragmentShaderSrc,
            NULL, 0 );

    FadedScreenMaskSquareDef.surfaceName = "FadedScreenMaskSquare";
    FadedScreenMaskSquareDef.geo = BuildFadedScreenMask( 0.0f, 0.0f );
    FadedScreenMaskSquareDef.graphicsCommand.GpuState.depthEnable = false;
    FadedScreenMaskSquareDef.graphicsCommand.GpuState.depthMaskEnable = false;
    FadedScreenMaskSquareDef.graphicsCommand.GpuState.cullEnable = false;
    FadedScreenMaskSquareDef.graphicsCommand.GpuState.colorMaskEnable[0] = false;
    FadedScreenMaskSquareDef.graphicsCommand.GpuState.colorMaskEnable[1] = false;
    FadedScreenMaskSquareDef.graphicsCommand.GpuState.colorMaskEnable[2] = false;
    FadedScreenMaskSquareDef.graphicsCommand.GpuState.colorMaskEnable[3] = true;

    BlackSceneScreenSurfaceDef.surfaceName = "SceneScreenSurf";
    //BlackSceneScreenSurfaceDef.geo = ; // Determined by the scene model loaded
    BlackSceneScreenSurfaceDef.graphicsCommand.Program = Cinema.ShaderMgr.ScenePrograms[0];

    OVR_LOG( "SceneManager::OneTimeInit: %3.1f seconds", GetTimeInSeconds() - start );
}

void SceneManager::OneTimeShutdown()
{
    OVR_LOG( "SceneManager::OneTimeShutdown" );

    // Free GL resources

    GlProgram::Free( FadedScreenMaskSquareDef.graphicsCommand.Program );
    FadedScreenMaskSquareDef.geo.Free();

    ScreenSurfaceDef.geo.Free();

    UnitSquare.Free();

    if ( ScreenVignetteTexture != 0 )
    {
        glDeleteTextures( 1, & ScreenVignetteTexture );
        ScreenVignetteTexture = 0;
    }

    if ( MipMappedMovieFBOs != NULL )
    {
        glDeleteFramebuffers( MipMappedMovieTextureSwapChainLength, MipMappedMovieFBOs );
        delete [] MipMappedMovieFBOs;
        MipMappedMovieFBOs = NULL;
    }

    if ( MipMappedMovieTextureSwapChain != NULL )
    {
        vrapi_DestroyTextureSwapChain( MipMappedMovieTextureSwapChain );
        MipMappedMovieTextureSwapChain = NULL;
        MipMappedMovieTextureSwapChainLength = 0;
    }

	SurfaceRender.Shutdown();
}

//=========================================================================================

static Vector3f GetMatrixUp( const Matrix4f & view )
{
	return Vector3f( view.M[0][1], view.M[1][1], view.M[2][1] );
}

static Vector3f GetMatrixForward( const Matrix4f & view )
{
	return Vector3f( -view.M[0][2], -view.M[1][2], -view.M[2][2] );
}

static float YawForMatrix( const Matrix4f &m )
{
	const Vector3f forward = GetMatrixForward( m );
	const Vector3f up = GetMatrixUp( m );

	float yaw;
	if ( forward.y > 0.7f )
	{
		yaw = atan2( up.x, up.z );
	}
	else if ( forward.y < -0.7f )
	{
		yaw = atan2( -up.x, -up.z );
	}
	else if ( up.y < 0.0f )
	{
		yaw = atan2( forward.x, forward.z );
	}
	else
	{
		yaw = atan2( -forward.x, -forward.z );
	}

	return yaw;
}

//=========================================================================================

// Sets the scene to the given model, sets the material on the
// model to the current sceneProgram for static / dynamic lighting,
// and updates:
// SceneScreenSurface
// SceneScreenBounds
// SeatPosition
void SceneManager::SetSceneModel( const SceneDef & sceneDef )
{
	OVR_LOG( "SetSceneModel %s", sceneDef.SceneModel->FileName.c_str() );

	VoidedScene = false;
	FixVoidedScene = false;

	SceneInfo = sceneDef;
	Scene.SetWorldModel( *SceneInfo.SceneModel );
	Scene.SetFreeMove( false );
	SceneScreenSurface = const_cast<ovrSurfaceDef *>( Scene.FindNamedSurface( "screen" ) );

	// Copy the geo off to the individual surface types we may need to render during the frame.
	if ( SceneScreenSurface != NULL )
	{
		BlackSceneScreenSurfaceDef.geo = SceneScreenSurface->geo;
	}

	for ( SceneSeatCount = 0; SceneSeatCount < MAX_SEATS; SceneSeatCount++ )
	{
		const ModelTag * tag = Scene.FindNamedTag( ( std::string( "cameraPos" ) + std::to_string( SceneSeatCount + 1 ) ).c_str() );
		if ( tag == NULL )
		{
			break;
		}
		SceneSeatPositions[SceneSeatCount] = tag->matrix.GetTranslation();
		SceneSeatPositions[SceneSeatCount].y -= Cinema.SceneMgr.Scene.GetEyeHeight();
	}

	if ( !sceneDef.UseSeats )
	{
		SceneSeatPositions[ SceneSeatCount ].z = 0.0f;
		SceneSeatPositions[ SceneSeatCount ].x = 0.0f;
		SceneSeatPositions[ SceneSeatCount ].y = 0.0f;
		SceneSeatCount++;
		SetSeat( 0 );
	}
	else if ( SceneSeatCount > 0 )
	{
		SetSeat( 0 );
	}
	else
	{
		// if no seats, create some at the position of the seats in home_theater
		for ( int seatPos = -1; seatPos <= 2; seatPos++ )
		{
			SceneSeatPositions[ SceneSeatCount ].z = 3.6f;
			SceneSeatPositions[ SceneSeatCount ].x = 3.0f + seatPos * 0.9f - 0.45f;
			SceneSeatPositions[ SceneSeatCount ].y = 0.0f;
			SceneSeatCount++;
		}

		for ( int seatPos = -1; seatPos <= 1; seatPos++ )
		{
			SceneSeatPositions[ SceneSeatCount ].z = 1.8f;
			SceneSeatPositions[ SceneSeatCount ].x = 3.0f + seatPos * 0.9f;
			SceneSeatPositions[ SceneSeatCount ].y = -0.3f;
			SceneSeatCount++;
		}
		SetSeat( 1 );
	}

	Scene.SetYawOffset( 0.0f );

	ClearGazeCursorGhosts();

	if ( SceneInfo.UseDynamicProgram )
	{
		SetSceneProgram( SceneProgramIndex, SCENE_PROGRAM_ADDITIVE );

		if ( SceneScreenSurface != NULL )
		{
			SceneScreenBounds = SceneScreenSurface->geo.localBounds;

			// force to a solid black material that cuts a hole in alpha
			SceneScreenSurface->graphicsCommand.Program = Cinema.ShaderMgr.ScenePrograms[0];
		}
	}

	FreeScreenActive = false;
	if ( SceneInfo.UseFreeScreen )
	{
		const float yaw = YawForMatrix( Scene.GetCenterEyeTransform() );
		FreeScreenPose = Matrix4f::RotationY( yaw ) * Matrix4f::Translation( Scene.GetCenterEyePosition() );
		FreeScreenActive = true;
	}
}

void SceneManager::SetSceneProgram( const sceneProgram_t opaqueProgram, const sceneProgram_t additiveProgram )
{
	if ( !Scene.GetWorldModel()->Definition || !SceneInfo.UseDynamicProgram )
	{
		return;
	}

	const GlProgram & dynamicOnlyProg = Cinema.ShaderMgr.ScenePrograms[SCENE_PROGRAM_DYNAMIC_ONLY];
	const GlProgram & opaqueProg = Cinema.ShaderMgr.ScenePrograms[opaqueProgram];
	const GlProgram & additiveProg = Cinema.ShaderMgr.ScenePrograms[additiveProgram];
	const GlProgram & diffuseProg = Cinema.ShaderMgr.ProgSingleTexture;

	OVR_LOG( "SetSceneProgram: %d(%d), %d(%d)", opaqueProgram, opaqueProg.Program, additiveProgram, additiveProg.Program );

	// Override the material on the background scene to allow the model to fade during state transitions.
	{
		const ModelFile * modelFile = Scene.GetWorldModel()->Definition;
		for ( int i = 0; i < static_cast< int >( modelFile->Models.size() ); i++ )
		{
			for ( int j = 0; j < static_cast< int >( modelFile->Models[i].surfaces.size() ); j++ )
			{
				if ( &modelFile->Models[i].surfaces[j].surfaceDef == SceneScreenSurface )
				{
					continue;
				}

				// FIXME: provide better solution for material overrides
				ovrGraphicsCommand & graphicsCommand = *const_cast< ovrGraphicsCommand * >( &modelFile->Models[i].surfaces[j].surfaceDef.graphicsCommand );
				
				if ( graphicsCommand.GpuState.blendSrc == GL_ONE && graphicsCommand.GpuState.blendDst == GL_ONE )
				{
					// Non-modulated additive material.
					if ( graphicsCommand.uniformTextures[1] != 0 )
					{
						graphicsCommand.uniformTextures[0] = graphicsCommand.uniformTextures[1];
						graphicsCommand.uniformTextures[1] = GlTexture();
					}

					graphicsCommand.Program = additiveProg;
				}
				else if ( graphicsCommand.uniformTextures[1] != 0 )
				{
					// Modulated material.
					if ( graphicsCommand.Program.Program != opaqueProg.Program &&
						( graphicsCommand.Program.Program == dynamicOnlyProg.Program ||
							opaqueProg.Program == dynamicOnlyProg.Program ) )
					{
						std::swap( graphicsCommand.uniformTextures[0], graphicsCommand.uniformTextures[1] );
					}

					graphicsCommand.Program = opaqueProg;
				}
				else
				{
					// Non-modulated diffuse material.
					graphicsCommand.Program = diffuseProg;
				}
			}
		}
	}

	SceneProgramIndex = opaqueProgram;
}

//=========================================================================================

static Vector3f ViewOrigin( const Matrix4f & view )
{
	return Vector3f( view.M[0][3], view.M[1][3], view.M[2][3] );
}

Posef SceneManager::GetScreenPose() const
{
	if ( FreeScreenActive )
	{
		const float applyScale = pow( 2.0f, FreeScreenScale );
		const Matrix4f screenMvp = FreeScreenPose *
				Matrix4f::Translation( 0, 0, -FreeScreenDistance*applyScale ) *
				Matrix4f::Scaling( applyScale, applyScale * (3.0f/4.0f), applyScale );

	return Posef( Quatf( FreeScreenPose ).Normalized(), ViewOrigin( screenMvp ) );
	}
	else
	{
		Vector3f pos = SceneScreenBounds.GetCenter();
		return Posef( Quatf(), pos );
	}
}

Vector2f SceneManager::GetScreenSize() const
{
	if ( FreeScreenActive )
	{
		Vector3f size = GetFreeScreenScale();
		return Vector2f( size.x * 2.0f, size.y * 2.0f );
	}
	else
	{
		Vector3f size = SceneScreenBounds.GetSize();
		return Vector2f( size.x, size.y );
	}
}

Vector3f SceneManager::GetFreeScreenScale() const
{
	// Scale is stored in a form that feels linear, raise to exponent to
	// get value to apply.
	const float applyScale = powf( 2.0f, FreeScreenScale );

	// adjust size based on aspect ratio
	float scaleX = 1.0f;
	float scaleY = ( CurrentMovieWidth == 0 ) ? 1.0f : (float)CurrentMovieHeight / CurrentMovieWidth;
	if ( scaleY > 0.6f )
	{
		scaleX *= 0.6f / scaleY;
		scaleY = 0.6f;
	}

	return Vector3f( applyScale * scaleX, applyScale * scaleY, applyScale );
}

Matrix4f SceneManager::FreeScreenMatrix() const
{
	const Vector3f scale = GetFreeScreenScale();
	return FreeScreenPose *
			Matrix4f::Translation( 0, 0, -FreeScreenDistance * scale.z ) *
			Matrix4f::Scaling( scale );
}

// Aspect is width / height
Matrix4f SceneManager::BoundsScreenMatrix( const Bounds3f & bounds, const float movieAspect ) const
{
	const Vector3f size = bounds.b[1] - bounds.b[0];
	const Vector3f center = bounds.b[0] + size * 0.5f;
	const float	screenHeight = size.y;
	const float screenWidth = std::max< float >( size.x, size.z );
	float widthScale;
	float heightScale;
	float aspect = ( movieAspect == 0.0f ) ? 1.0f : movieAspect;
	if ( screenWidth / screenHeight > aspect )
	{	// screen is wider than movie, clamp size to height
		heightScale = screenHeight * 0.5f;
		widthScale = heightScale * aspect;
	}
	else
	{	// screen is taller than movie, clamp size to width
		widthScale = screenWidth * 0.5f;
		heightScale = widthScale / aspect;
	}

	const float rotateAngle = ( size.x > size.z ) ? 0.0f : MATH_FLOAT_PI * 0.5f;

	return	Matrix4f::Translation( center ) *
			Matrix4f::RotationY( rotateAngle ) *
			Matrix4f::Scaling( widthScale, heightScale, 1.0f );
}

Matrix4f SceneManager::ScreenMatrix() const
{
	if ( FreeScreenActive )
	{
		return FreeScreenMatrix();
	}
	else
	{
		return BoundsScreenMatrix( SceneScreenBounds,
			( CurrentMovieHeight == 0 ) ? 1.0f : ( (float)CurrentMovieWidth / CurrentMovieHeight ) );
	}
}

void SceneManager::ClearMovie()
{
	Native::StopMovie();

	SetSceneProgram( SCENE_PROGRAM_DYNAMIC_ONLY, SCENE_PROGRAM_ADDITIVE );

	MovieTextureTimestamp = 0;
	FrameUpdateNeeded = true;
	CurrentMovieWidth = 0;

	delete MovieTexture;
	MovieTexture = NULL;
}

void SceneManager::PutScreenInFront()
{
	FreeScreenPose = Scene.GetCenterEyeTransform();
	//Cinema.app->RecenterYaw( false );
}

void SceneManager::ClearGazeCursorGhosts()
{
	// clear gaze cursor to avoid seeing it lerp
	ClearGhostsFrames = 3;
}

void SceneManager::LightsOn( const float duration, const double currTimeInSeconds )
{
	StaticLighting.Set( currTimeInSeconds, StaticLighting.Value( currTimeInSeconds ), currTimeInSeconds + duration, 1.0 );
}

void SceneManager::LightsOff( const float duration, const double currTimeInSeconds )
{
	StaticLighting.Set( currTimeInSeconds, StaticLighting.Value( currTimeInSeconds ), currTimeInSeconds + duration, 0.0 );
}

void SceneManager::CheckForbufferResize()
{
	if ( BufferSize.x == MovieTextureWidth && BufferSize.y == MovieTextureHeight )
	{	// already the correct size
		return;
	}

	if ( MovieTextureWidth <= 0 || MovieTextureHeight <= 0 )
	{	// don't try to change to an invalid size
		return;
	}
	BufferSize.x = MovieTextureWidth;
	BufferSize.y = MovieTextureHeight;

	// Free previously created frame buffers and texture set.
	if ( MipMappedMovieFBOs != NULL )
	{
		glDeleteFramebuffers( MipMappedMovieTextureSwapChainLength, MipMappedMovieFBOs );
		delete [] MipMappedMovieFBOs;
		MipMappedMovieFBOs = NULL;
	}
	if ( MipMappedMovieTextureSwapChain != NULL )
	{
		vrapi_DestroyTextureSwapChain( MipMappedMovieTextureSwapChain );
		MipMappedMovieTextureSwapChain = NULL;
		MipMappedMovieTextureSwapChainLength = 0;
	}

	// Create the texture set that we will mip map from the external image.
	MipMappedMovieTextureSwapChain = vrapi_CreateTextureSwapChain3( VRAPI_TEXTURE_TYPE_2D,
											Cinema.GetUseSrgb() ? GL_SRGB8_ALPHA8 : GL_RGBA8,
											MovieTextureWidth, MovieTextureHeight,
											ComputeFullMipChainNumLevels( MovieTextureWidth, MovieTextureHeight ), 3 );
	MipMappedMovieTextureSwapChainLength = vrapi_GetTextureSwapChainLength( MipMappedMovieTextureSwapChain );

	MipMappedMovieFBOs = new GLuint[MipMappedMovieTextureSwapChainLength];
	glGenFramebuffers( MipMappedMovieTextureSwapChainLength, MipMappedMovieFBOs );
	for ( int i = 0; i < MipMappedMovieTextureSwapChainLength; i++ )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, MipMappedMovieFBOs[i] );
		glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, vrapi_GetTextureSwapChainHandle( MipMappedMovieTextureSwapChain, i ), 0 );
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}
}

//============================================================================================

GLuint SceneManager::BuildScreenVignetteTexture( const int horizontalTile ) const
{
	// make it an even border at 16:9 aspect ratio, let it get a little squished at other aspects
	static const int scale = 24;
	static const int width = 16 * scale * horizontalTile;
	static const int height = 9 * scale;
	unsigned char * buffer = new unsigned char[width * height];
	memset( buffer, 255, sizeof( unsigned char ) * width * height );
	for ( int i = 0; i < width; i++ )
	{
		buffer[i] = 0;						// horizontal black top
		buffer[width*height - 1 - i] = 0;	// horizontal black bottom
	}
	for ( int i = 0; i < height; i++ )
	{
		buffer[i * width] = 0;				 // vertical black left
		buffer[i * width + width - 1] = 0;	 // vertical black right
		if ( horizontalTile == 2 )			 // vertical black middle
		{
			buffer[i * width + width / 2 - 1] = 0;
			buffer[i * width + width / 2] = 0;
		}
	}
	GLuint texId;
	glGenTextures( 1, &texId );
	glBindTexture( GL_TEXTURE_2D, texId );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, buffer );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glBindTexture( GL_TEXTURE_2D, 0 );

	delete[] buffer;
	GLCheckErrorsWithTitle( "screenVignette" );
	return texId;
}

int SceneManager::BottomMipLevel( const int width, const int height ) const
{
	int bottomMipLevel = 0;
	int dimension = std::max< int >( width, height );

	while( dimension > 1 )
	{
		bottomMipLevel++;
		dimension >>= 1;
	}

	return bottomMipLevel;
}

void SceneManager::SetSeat( int newSeat )
{
	SeatPosition = newSeat;
	Scene.SetFootPos( SceneSeatPositions[ SeatPosition ] );
}


/*
 * Command
 *
 * Actions that need to be performed on the render thread.
 */
bool SceneManager::Command( const char * msg )
{
	// Always include the space in MatchesHead to prevent problems
	// with commands with matching prefixes.
	OVR_LOG( "SceneManager::Command: %s", msg );

	if ( MatchesHead( "newVideo ", msg ) )
	{
		delete MovieTexture;
		const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( Cinema.GetContext()->ContextForVrApi() ) );
		MovieTexture = new SurfaceTexture( ctx.Env );
		OVR_LOG( "RC_NEW_VIDEO texId %i", MovieTexture->GetTextureId() );

		ovrMessageQueue * receiver;
		sscanf( msg, "newVideo %p", &receiver );

		receiver->PostPrintf( "surfaceTexture %p", MovieTexture->GetJavaObject() );

		// don't draw the screen until we have the new size
		CurrentMovieWidth = 0;
		return true;
	}

	if ( MatchesHead( "video ", msg ) )
	{
		int width, height;
		sscanf( msg, "video %i %i", &width, &height);

		MovieTextureWidth = width;
		MovieTextureHeight = height;

		long numberOfPixels = MovieTextureWidth * MovieTextureHeight;
		OVR_LOG( "Movie size: %dx%d = %ld pixels", MovieTextureWidth, MovieTextureHeight, numberOfPixels );

		// use the void theater on large movies
		if ( numberOfPixels > 1920 * 1080 )
		{
			OVR_LOG( "Oversized movie.  Switching to Void scene to reduce judder" );
			SetSceneModel( *Cinema.ModelMgr.VoidScene );
			PutScreenInFront();
			//UseOverlay = true;
			VoidedScene = true;
			FixVoidedScene = true;

			// downsize the screen resolution to fit into a 960x540 buffer
			/*float aspectRatio = ( float )width / ( float )height;
			if ( aspectRatio < 1.0f )
			{
				MovieTextureWidth = static_cast<int>( aspectRatio * 540.0f );
				MovieTextureHeight = 540;
			}
			else
			{
				MovieTextureWidth = 960;
				MovieTextureHeight = static_cast<int>( aspectRatio * 960.0f );
			}*/
		}

		CurrentMovieWidth = width;
		CurrentMovieHeight = height;
		FrameUpdateNeeded = true;
		return true;
	}

	return false;
}

void SceneManager::AppRenderFrame( const ovrApplFrameIn & in, ovrRendererOutput & out )
{
	// disallow player movement
	ovrApplFrameIn vrFrameWithoutMove = in;
	vrFrameWithoutMove.LeftRemote.Joystick.x = 0.0f;
	vrFrameWithoutMove.LeftRemote.Joystick.y = 0.0f;

	// Suppress scene surfaces when Free Screen is active.
	Scene.Frame( vrFrameWithoutMove, SceneInfo.UseFreeScreen ? 0 : -1 );

	// For some reason the void screen is placed in the wrong position when
	// we switch to it for videos that are too costly to decode while drawing
	// the scene (which causes judder).
	if ( FixVoidedScene )
	{
		PutScreenInFront();
		FixVoidedScene = false;
	}

	if ( ClearGhostsFrames > 0 )
	{
		Cinema.GetGuiSys().GetGazeCursor().ClearGhosts();
		ClearGhostsFrames--;
	}

	// Check for new movie frames
	// latch the latest movie frame to the texture.
	if ( MovieTexture != NULL && CurrentMovieWidth != 0 )
	{
	    glActiveTexture( GL_TEXTURE0 );
	    MovieTexture->Update();
	    glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );
	    if ( MovieTexture->GetNanoTimeStamp() != MovieTextureTimestamp )
	    {
			MovieTextureTimestamp = MovieTexture->GetNanoTimeStamp();
			FrameUpdateNeeded = true;
	    }
	    FrameUpdateNeeded = true;
	    Cinema.MovieScreenUpdated();
	}

	CheckForbufferResize();

	// build the mip maps
	if ( FrameUpdateNeeded && MipMappedMovieTextureSwapChain != NULL )
	{
		FrameUpdateNeeded = false;
		CurrentMipMappedMovieTexture = ( CurrentMipMappedMovieTexture + 1 ) % MipMappedMovieTextureSwapChainLength;
		glActiveTexture( GL_TEXTURE1 );
		glBindTexture( GL_TEXTURE_2D, ScreenVignetteTexture );
		glActiveTexture( GL_TEXTURE0 );
		glBindFramebuffer( GL_FRAMEBUFFER, MipMappedMovieFBOs[CurrentMipMappedMovieTexture] );
		glDisable( GL_DEPTH_TEST );
		glDisable( GL_SCISSOR_TEST );
		GL_InvalidateFramebuffer( INV_FBO, true, false );
		glViewport( 0, 0, MovieTextureWidth, MovieTextureHeight );
		if ( Cinema.GetUseSrgb() )
		{	// we need this copied without sRGB conversion on the top level
			glDisable( GL_FRAMEBUFFER_SRGB_EXT );
		}
		if ( CurrentMovieWidth > 0 )
		{
			glBindTexture( GL_TEXTURE_EXTERNAL_OES, MovieTexture->GetTextureId() );
			glUseProgram( Cinema.ShaderMgr.CopyMovieProgram.Program );
			glBindVertexArray( UnitSquare.vertexArrayObject );
			glDrawElements( UnitSquare.primitiveType, UnitSquare.indexCount, UnitSquare.IndexType, NULL );
			glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );
			if ( Cinema.GetUseSrgb() )
			{	// we need this copied without sRGB conversion on the top level
				glEnable( GL_FRAMEBUFFER_SRGB_EXT );
			}
		}
		else
		{
			// If the screen is going to be black because of a movie change, don't
			// leave the last dynamic color visible.
			glClearColor( 0.2f, 0.2f, 0.2f, 0.2f );
			glClear( GL_COLOR_BUFFER_BIT );
		}
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );

		// texture 2 will hold the mip mapped screen
		glActiveTexture( GL_TEXTURE2 );
		glBindTexture( GL_TEXTURE_2D, vrapi_GetTextureSwapChainHandle( MipMappedMovieTextureSwapChain, CurrentMipMappedMovieTexture ) );
		glGenerateMipmap( GL_TEXTURE_2D );
		glBindTexture( GL_TEXTURE_2D, 0 );

		GL_Flush();
	}

	// Apply the scene lighting
	if ( SceneInfo.UseDynamicProgram )
	{
		// lights fading in and out, always on if no movie loaded
		const float cinemaLights = ( ( MovieTextureWidth > 0 ) && !SceneInfo.UseFreeScreen ) ?
				(float)StaticLighting.Value( in.RealTimeInSeconds ) : 1.0f;

		if ( cinemaLights <= 0.0f )
		{
			if ( SceneProgramIndex != SCENE_PROGRAM_DYNAMIC_ONLY )
			{
				// switch to dynamic-only to save GPU
				SetSceneProgram( SCENE_PROGRAM_DYNAMIC_ONLY, SCENE_PROGRAM_ADDITIVE );
			}
		}
		else if ( cinemaLights >= 1.0f )
		{
			if ( SceneProgramIndex != SCENE_PROGRAM_STATIC_ONLY )
			{
				// switch to static-only to save GPU
				SetSceneProgram( SCENE_PROGRAM_STATIC_ONLY, SCENE_PROGRAM_ADDITIVE );
			}
		}
		else
		{
			if ( SceneProgramIndex != SCENE_PROGRAM_STATIC_DYNAMIC )
			{
				// switch to static+dynamic lighting
				SetSceneProgram( SCENE_PROGRAM_STATIC_DYNAMIC, SCENE_PROGRAM_ADDITIVE );
			}
		}

		// For Dynamic and Static-Dynamic, set the mip mapped movie texture to Texture2
		// so it can be sampled from the vertex program for scene lighting.
		const unsigned int lightingTexId = ( MipMappedMovieTextureSwapChain != NULL ) ? 
			vrapi_GetTextureSwapChainHandle( MipMappedMovieTextureSwapChain, CurrentMipMappedMovieTexture ) : 0;

		// Override the material on the background scene to allow the model to fade during state transitions.
		{
			const ModelFile * modelFile = Scene.GetWorldModel()->Definition;
			for ( int i = 0; i < static_cast< int >( modelFile->Models.size() ); i++ )
			{
				for ( int j = 0; j < static_cast< int >( modelFile->Models[i].surfaces.size() ); j++ )
				{
					if ( &modelFile->Models[i].surfaces[j].surfaceDef == SceneScreenSurface )
					{
						continue;
					}

					// FIXME: provide better solution for material overrides
					ovrGraphicsCommand & graphicsCommand = *const_cast< ovrGraphicsCommand * >( &modelFile->Models[i].surfaces[j].surfaceDef.graphicsCommand );
					graphicsCommand.uniformSlots[0] = graphicsCommand.Program.uColor;
					graphicsCommand.uniformValues[0][0] = 1.0f;
					graphicsCommand.uniformValues[0][1] = 1.0f;
					graphicsCommand.uniformValues[0][2] = 1.0f;
					graphicsCommand.uniformValues[0][3] = cinemaLights;

					// Do not try to apply the scene lighting texture if it is not valid.
					if ( lightingTexId != 0 )
					{
						graphicsCommand.numUniformTextures = 3;
						graphicsCommand.uniformTextures[2] = GlTexture( lightingTexId, 0, 0 );
					}
				}
			}
		}
	}

	Scene.GetFrameMatrices( Cinema.GetSuggestedEyeFovDegreesX(), Cinema.GetSuggestedEyeFovDegreesY(), out.FrameMatrices );
	Scene.GenerateFrameSurfaceList( out.FrameMatrices, out.Surfaces );

	//-------------------------------
	// Rendering
	//-------------------------------
	out.FrameFlags = 0;

	const bool drawScreen = ( ( SceneScreenSurface != NULL ) || SceneInfo.UseFreeScreen ) && MovieTexture && ( CurrentMovieWidth > 0 );

	// Draw the movie screen first and compose the eye buffers on top (with holes in alpha)
	if ( drawScreen )
	{
		// If we are using the free screen, we still need to draw the surface with black
		if ( FreeScreenActive && !SceneInfo.UseFreeScreen && ( SceneScreenSurface != NULL ) )
		{
			BlackSceneScreenSurfaceDef.graphicsCommand.GpuState.depthEnable = false;
			out.Surfaces.push_back( ovrDrawSurface( &BlackSceneScreenSurfaceDef ) );
		}

		Matrix4f texMatrix[2];
		ovrRectf texRect[2];
		for ( int eye = 0; eye < 2; eye++ )
		{
			texMatrix[eye] = Matrix4f::Identity();
			texRect[eye] = { 0.0f, 0.0f, 1.0f, 1.0f };
		}

		// Draw the movie texture layer
		{
			ovrLayerProjection2 & overlayLayer = out.Layers[ out.NumLayers++ ].Projection;
			overlayLayer = vrapi_DefaultLayerProjection2();

			overlayLayer.Header.SrcBlend = VRAPI_FRAME_LAYER_BLEND_ONE;
			overlayLayer.Header.DstBlend = VRAPI_FRAME_LAYER_BLEND_ZERO;
			overlayLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

			overlayLayer.HeadPose = Cinema.GetFrame().Tracking.HeadPose;
			for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++ )
			{
				const ovrMatrix4f modelViewMatrix = Scene.GetEyeViewMatrix( eye ) * ScreenMatrix();
				overlayLayer.Textures[eye].ColorSwapChain = MipMappedMovieTextureSwapChain;
				overlayLayer.Textures[eye].SwapChainIndex = CurrentMipMappedMovieTexture;
				overlayLayer.Textures[eye].TexCoordsFromTanAngles = texMatrix[eye] * ovrMatrix4f_TanAngleMatrixFromUnitSquare( &modelViewMatrix );
				overlayLayer.Textures[eye].TextureRect = texRect[eye];
			}
		}

		// explicitly clear a hole in the eye buffer alpha
		{
			out.Surfaces.push_back( ovrDrawSurface( ScreenMatrix(), &FadedScreenMaskSquareDef ) );
		}
	}

	ovrLayerProjection2 & worldLayer = out.Layers[ out.NumLayers++ ].Projection;
	worldLayer = vrapi_DefaultLayerProjection2();

	worldLayer.Header.SrcBlend = ( drawScreen ) ? VRAPI_FRAME_LAYER_BLEND_SRC_ALPHA : VRAPI_FRAME_LAYER_BLEND_ONE;
	worldLayer.Header.DstBlend = ( drawScreen ) ? VRAPI_FRAME_LAYER_BLEND_ONE_MINUS_SRC_ALPHA : VRAPI_FRAME_LAYER_BLEND_ZERO;
	worldLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

	worldLayer.HeadPose = Cinema.GetFrame().Tracking.HeadPose;
	for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++ )
	{
		ovrFramebuffer * framebuffer = Cinema.GetFrameBuffer( eye );
		worldLayer.Textures[eye].ColorSwapChain = framebuffer->ColorTextureSwapChain;
		worldLayer.Textures[eye].SwapChainIndex = framebuffer->TextureSwapChainIndex;
		worldLayer.Textures[eye].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection( (ovrMatrix4f *)&out.FrameMatrices.EyeProjection[eye] );
	}

	// render images for each eye
	for ( int eye = 0; eye < Cinema.GetNumFramebuffers(); ++eye )
	{
		ovrFramebuffer * framebuffer = Cinema.GetFrameBuffer( eye );
		ovrFramebuffer_SetCurrent( framebuffer );

		Cinema.AppEyeGLStateSetup( in, framebuffer, eye );
		AppRenderEye( in, out, eye );

		ovrFramebuffer_Resolve( framebuffer );
		ovrFramebuffer_Advance( framebuffer );
	}

	ovrFramebuffer_SetNone();
}

void SceneManager::AppRenderEye( const ovrApplFrameIn & in, ovrRendererOutput & out, int eye )
{
	// Render the surfaces returned by Frame.
	SurfaceRender.RenderSurfaceList(
			out.Surfaces,
			out.FrameMatrices.EyeView[0], 		// always use 0 as it assumes an array
			out.FrameMatrices.EyeProjection[0], // always use 0 as it assumes an array
			eye );
}

} // namespace OculusCinema
