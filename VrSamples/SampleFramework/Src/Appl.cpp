/*******************************************************************************

Filename    :   Appl.cpp
Content     :   VR application base class.
Created     :   March 20, 2018
Authors     :   Jonathan Wright
Language    :   C++

Copyright	:	Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*******************************************************************************/

#include "Appl.h"
#include "CompilerUtils.h"

#include "OVR_Math.h"
#include "JniUtils.h"

#include "Platform/Android/Android.h"

#include <android/window.h>
#include <android/native_window_jni.h>

#include <memory>

#include "VrApi_SystemUtils.h"

// android_handle_cmd
//==============================================================
void android_handle_cmd( struct android_app * app, int32_t cmd )
{
	OVRFW::ovrAppl * appl = (OVRFW::ovrAppl*)app->userData;
	if ( appl == nullptr )
	{
		ALOGV( "android_handle_cmd with NULL user data!" );
		return;
	}

	switch( cmd )
	{
	case APP_CMD_START:
		ALOGV( "APP_CMD_START" );
		break;
	case APP_CMD_RESUME:
		ALOGV( "APP_CMD_RESUME" );
		appl->SetLifecycle( OVRFW::ovrAppl::LIFECYCLE_RESUMED );
		break;
	case APP_CMD_PAUSE:
		ALOGV( "APP_CMD_PAUSE" );
		appl->SetLifecycle( OVRFW::ovrAppl::LIFECYCLE_PAUSED );
		break;
	case APP_CMD_STOP:
		ALOGV( "APP_CMD_STOP" );
		break;
	case APP_CMD_DESTROY:
		ALOGV( "APP_CMD_DESTROY" );
		appl->SetWindow( nullptr );
		break;
	case APP_CMD_INIT_WINDOW:
		ALOGV( "APP_CMD_INIT_WINDOW" );
		appl->SetWindow( app->window );
		break;
	case APP_CMD_TERM_WINDOW:
		ALOGV( "APP_TERM_WINDOW" );
		appl->SetWindow( nullptr );
		break;
	}
}

//==============================================================
// android_handle_input
//
// TODO: This handles events that come through normal Android input 
// routing, such as keyboard keys. Headset and controller already
// come through VrApi input APIs, so we should not be encouraging
// this route. This is left in for reference in the initial 
// check in, but will be stubbed in later change sets.
//==============================================================
int32_t android_handle_input( struct android_app * app, AInputEvent * event )
{
	OVRFW::ovrAppl * appl = (OVRFW::ovrAppl*)app->userData;
	if ( appl == nullptr )
	{
		ALOGV( "android_handle_input with NULL user data!" );
		return 0;
	}

	const int type = AInputEvent_getType( event );
	if ( type == AINPUT_EVENT_TYPE_KEY )
	{
		const int keyCode = AKeyEvent_getKeyCode( event );
		const int action = AKeyEvent_getAction( event );
		appl->AddKeyEvent( keyCode, action );
		return 1; // we eat all other key events
	}
	else if ( type == AINPUT_EVENT_TYPE_MOTION )
	{
		const int action = AKeyEvent_getAction( event ) & AMOTION_EVENT_ACTION_MASK;
		const float x = AMotionEvent_getRawX( event, 0 );
		const float y = AMotionEvent_getRawY( event, 0 );
		appl->AddTouchEvent( action, x, y );
		return 1; // we eat all touch events
	}
	return 0;
}

namespace OVRFW {
void ovrAppl::SetLifecycle( const ovrLifecycle lc )
{
	Lifecycle = lc;
}

ovrAppl::ovrLifecycle ovrAppl::GetLifecycle() const
{
	return Lifecycle;
}

void ovrAppl::SetWindow( const void * window )
{
	Window = window;
}

const void * ovrAppl::GetWindow() const
{
	return Window;
}

// set the clock levels immediately, as long as there is a valid VR session.
void ovrAppl::SetClockLevels( const int32_t cpuLevel, const int32_t gpuLevel )
{
	CpuLevel = cpuLevel;
	GpuLevel = gpuLevel;
	vrapi_SetClockLevels( SessionObject, cpuLevel, gpuLevel );
}

void ovrAppl::GetClockLevels( int32_t & cpuLevel, int & gpuLevel ) const
{
	cpuLevel = CpuLevel;
	gpuLevel = GpuLevel;
}

bool ovrAppl::Init( const ovrAppContext * context, const ovrInitParms * initParms )
{
	ALOGV( "ovrAppl::Init" );
	Context = context;
	const ovrJava * java = reinterpret_cast< const ovrJava* >( context->ContextForVrApi() );

	int32_t result = VRAPI_INITIALIZE_SUCCESS;
	if ( initParms == nullptr )
	{
		const ovrInitParms localInitParms = vrapi_DefaultInitParms( java );
		result = vrapi_Initialize( &localInitParms );
	}
	else
	{
		result = vrapi_Initialize( initParms );
	}

	if ( result != VRAPI_INITIALIZE_SUCCESS )
	{
		ALOGV( "ovrAppl::Init - failed to Initialize initParms=%p", initParms );
		return false;
	}

	ovrEgl_Clear( &Egl );
	ovrEgl_CreateContext( &Egl, NULL );
	EglInitExtensions();

	SuggestedEyeFovDegreesX = vrapi_GetSystemPropertyFloat( java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X );
	SuggestedEyeFovDegreesY = vrapi_GetSystemPropertyFloat( java, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y );

	ALOGV( "ovrAppl::Init - Use Multiview: %s", UseMultiView ? "true" : "false" );
	GlProgram::SetUseMultiview( UseMultiView );

	// Init renderer
	static const int NUM_MULTI_SAMPLES = 4;
	for ( int eye = 0; eye < NumFramebuffers; eye++ )
	{
		Framebuffer[eye] = std::unique_ptr<ovrFramebuffer>( new ovrFramebuffer() );
		ovrFramebuffer_Clear( Framebuffer[eye].get() );
		ovrFramebuffer_Create( Framebuffer[eye].get(), UseMultiView, GL_RGBA8,
		                       vrapi_GetSystemPropertyInt( java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH ),
		                       vrapi_GetSystemPropertyInt( java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT ),
		                       NUM_MULTI_SAMPLES );
	}

	bool appResult = AppInit( context );
	if ( !appResult )
	{
		AppShutdown( context );
	}

	ALOGV( "ovrAppl::Init - ALL DONE" );
	return appResult;
}

bool ovrAppl::AppInit( const ovrAppContext * /* context */ )
{
	ALOGV( "Default AppInit called!" );
	return true;
}

void ovrAppl::Shutdown( const ovrAppContext * context )
{
	Context = nullptr;
	AppShutdown( context );

	for ( int eye = 0; eye < NumFramebuffers; eye++ )
	{
		ovrFramebuffer_Destroy( Framebuffer[eye].get() );
	}

	ovrEgl_DestroyContext( &Egl );

	vrapi_Shutdown();
}

void ovrAppl::AppShutdown( const ovrAppContext * /* context */ )
{
	ALOGV( "Default AppShutdown called!" );
}

void ovrAppl::AppResumed( const ovrAppContext * /* context */ )
{

}

void ovrAppl::AppPaused( const ovrAppContext * /* context */ )
{

}

void ovrAppl::AppHandleInputShutdownRequest( ovrRendererOutput & out )
{
	/// a back click happened - return to home
	vrapi_ShowSystemUI( 
		reinterpret_cast< const ovrJava* >( Context->ContextForVrApi() ), 
		VRAPI_SYS_UI_CONFIRM_QUIT_MENU );
	/// render black 
	ovrLayerProjection2 blackLayer = vrapi_DefaultLayerBlackProjection2();
	blackLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;
	out.Layers[out.NumLayers++].Projection = blackLayer;
	out.FrameFlags |= VRAPI_FRAME_FLAG_FLUSH;
	out.FrameFlags |= VRAPI_FRAME_FLAG_FINAL;
}

static void SubmitLoadingIcon( ovrMobile * sessionObject,
                               const uint64_t frameIndex, const double displayTime )
{
	ovrLayer_Union2 layers[ovrMaxLayerCount] = {};

	// black layer
	ovrLayerProjection2 blackLayer = vrapi_DefaultLayerBlackProjection2();
	blackLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;
	layers[0].Projection = blackLayer;
	// loading icon layer
	ovrLayerLoadingIcon2 iconLayer = vrapi_DefaultLayerLoadingIcon2();
	iconLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;
	layers[1].LoadingIcon = iconLayer;

	const ovrLayerHeader2 * layerPtrs[ovrMaxLayerCount] = { &layers[0].Header, &layers[1].Header };

	ovrSubmitFrameDescription2 frameDesc = {};
	frameDesc.Flags = VRAPI_FRAME_FLAG_FLUSH;
	frameDesc.SwapInterval = 1;
	frameDesc.FrameIndex = frameIndex;
	frameDesc.DisplayTime = displayTime;
	frameDesc.LayerCount = 2;
	frameDesc.Layers = layerPtrs;

	vrapi_SubmitFrame2( sessionObject, &frameDesc );
}

void ovrAppl::HandleLifecycle( const ovrAppContext * context )
{
	if ( Lifecycle == LIFECYCLE_UNKNOWN )
	{
		ALOGV( "LIFECYCLE_UNKNOWN" );
		return;
	}

	if ( Lifecycle == LIFECYCLE_RESUMED && Window != nullptr )
	{
		if ( SessionObject == nullptr )
		{
			ovrModeParms modeParms = vrapi_DefaultModeParms( reinterpret_cast< const ovrJava* >( context->ContextForVrApi() ) );

			modeParms.Flags &= ~VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;
			modeParms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
			modeParms.Display = (size_t)Egl.Display;
			modeParms.WindowSurface = (size_t)Window;
			modeParms.ShareContext = (size_t)Egl.Context;

			ALOGV( "eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );
			ALOGV( "vrapi_EnterVrMode()" );

			SessionObject = vrapi_EnterVrMode( &modeParms );

			ALOGV( "eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );

			if ( SessionObject == nullptr )
			{
				ALOGE( "Invalid ANativeWindow!" );
				Window = nullptr;
			}
			else
			{
				vrapi_SetClockLevels( SessionObject, CpuLevel, GpuLevel );
				if ( MainThreadTid > 0 )
				{
					vrapi_SetPerfThread( SessionObject, VRAPI_PERF_THREAD_TYPE_MAIN, MainThreadTid );
				}
				if ( RenderThreadTid > 0 )
				{
					vrapi_SetPerfThread( SessionObject, VRAPI_PERF_THREAD_TYPE_RENDERER, RenderThreadTid );
				}
				vrapi_SetTrackingSpace( SessionObject, VRAPI_TRACKING_SPACE_LOCAL_FLOOR );
			}

			SubmitLoadingIcon( SessionObject, GetFrameIndex(), GetDisplayTime() );
			AppResumed( context );
		}
	}
	else
	{
		if ( SessionObject != nullptr )
		{
			// TODO: if separate render thread, wait on render thread before leaving VR mode
			AppPaused( context );

			ALOGV( "eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );
			ALOGV( "vrapi_LeaveVrMode()" );

			vrapi_LeaveVrMode( SessionObject );
			SessionObject = nullptr;

			ALOGV( "eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface( EGL_DRAW ) );
		}
	}
}

bool ovrAppl::HasActiveVRSession() const
{
	return SessionObject != nullptr;
}

void ovrAppl::ChangeVolume( int volumeDelta )
{
	ALOGV( "ovrAppl::ChangeVolume %d", volumeDelta );

/// Call the Volume API like the Java method below throw JNI
/*
	private void adjustVolume(int direction) {
		AudioManager audio = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
		audio.adjustStreamVolume(AudioManager.STREAM_MUSIC, direction, 0);
	}
*/

	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( GetContext()->ContextForVrApi() ) );
	TempJniEnv env( ctx.Vm );
	jobject me = ctx.ActivityObject;

	jclass contextClass = env->FindClass("android/content/Context");
	if ( 0 == contextClass )
	{
		ALOGV( "contextClass = %p ", (void *)contextClass );
		return;
	}

	jclass audioManagerClass = env->FindClass("android/media/AudioManager");
	if ( 0 == audioManagerClass )
	{
		ALOGV( "audioManagerClass = %p ", (void *)audioManagerClass );
		return;
	}

	jfieldID asField = env->GetStaticFieldID(contextClass, "AUDIO_SERVICE", "Ljava/lang/String;");
	if ( 0 == asField )
	{
		ALOGV( "asField = %p ", (void *)asField );
		return;
	}

	jmethodID getSystemServiceID = env->GetMethodID(contextClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
	if ( 0 == getSystemServiceID )
	{
		ALOGV( "getSystemServiceID = %p ", (void *)getSystemServiceID );
		return;
	}

	jmethodID adjustStreamVolumeID = env->GetMethodID(audioManagerClass, "adjustStreamVolume", "(III)V");
	if ( 0 == adjustStreamVolumeID )
	{
		ALOGV( "adjustStreamVolumeID = %p ", (void *)adjustStreamVolumeID );
		return;
	}

	jstring audioManagerName = (jstring) env->GetStaticObjectField(contextClass, asField );
	if ( 0 == audioManagerName )
	{
		ALOGV( "audioManagerName =  %p ", (void *)audioManagerName ) ;
		return;
	}

	jobject audioManagerObject = env->CallObjectMethod(me, getSystemServiceID, audioManagerName);
	if ( 0 == audioManagerObject )
	{
		ALOGV( "audioManagerObject = %p ", (void *)audioManagerObject );
		return;
	}
	env->CallVoidMethod(audioManagerObject, adjustStreamVolumeID, 3 /* STREAM_MUSIC */, volumeDelta, 0 );
}

ovrApplFrameOut ovrAppl::Frame( ovrApplFrameIn & frameIn )
{
	assert( SessionObject != nullptr );

	// This is the only place frame index is incremented, right before getting the
	// predicted display time
	FrameIndex++;

	const ovrPosef trackingPose = vrapi_LocateTrackingSpace( SessionObject, vrapi_GetTrackingSpace( SessionObject ) );
	const ovrPosef eyeLevelTrackingPose = vrapi_LocateTrackingSpace( SessionObject, VRAPI_TRACKING_SPACE_LOCAL );

	frameIn.FrameIndex = FrameIndex;
	frameIn.PredictedDisplayTime = vrapi_GetPredictedDisplayTime( SessionObject, FrameIndex );
	frameIn.RealTimeInSeconds = vrapi_GetTimeInSeconds();
	frameIn.Tracking = vrapi_GetPredictedTracking2( SessionObject, frameIn.PredictedDisplayTime );
	frameIn.EyeHeight = vrapi_GetEyeHeight( &eyeLevelTrackingPose, &trackingPose );
	frameIn.IPD = vrapi_GetInterpupillaryDistance( &frameIn.Tracking );
	const ovrJava * java = reinterpret_cast< const ovrJava* >( Context->ContextForVrApi() );
	frameIn.RecenterCount = vrapi_GetSystemStatusInt( java, VRAPI_SYS_STATUS_RECENTER_COUNT );

	static double LastPredictedDisplayTime = frameIn.PredictedDisplayTime;
	frameIn.DeltaSeconds = float( frameIn.PredictedDisplayTime - LastPredictedDisplayTime );
	LastPredictedDisplayTime = frameIn.PredictedDisplayTime;

	// save the display time for the renderer
	DisplayTime = frameIn.PredictedDisplayTime;

	// copy input from the pending lists into the frame input
	{
		std::lock_guard< std::mutex > lock( KeyEventMutex );
		frameIn.KeyEvents = PendingKeyEvents;
		PendingKeyEvents.clear();
	}
	{
		std::lock_guard< std::mutex > lock( TouchEventMutex );
		frameIn.TouchEvents = PendingTouchEvents;
		PendingTouchEvents.clear();
	}

	// VR input 
	HandleVREvents( frameIn );

	///  From https://developer.android.com/reference/android/view/KeyEvent.html#KEYCODE_VOLUME_UP
	static const int32_t KEYCODE_VOLUME_DOWN = 0x00000019;
	static const int32_t KEYCODE_VOLUME_UP = 0x00000018;

	// Handle Volume events
	for ( const ovrKeyEvent& event : frameIn.KeyEvents )
	{
		if ( event.KeyCode == KEYCODE_VOLUME_UP )
		{
			ChangeVolume( +1 );
		}
		if ( event.KeyCode == KEYCODE_VOLUME_DOWN )
		{
			ChangeVolume( -1 );
		}
	}

#if 0
	// Hack to mono 
	frameIn.Tracking.Eye[1] = frameIn.Tracking.Eye[0]; // All L
	// frameIn.Tracking.Eye[0] = frameIn.Tracking.Eye[1]; // All R
#endif

#if 0
	// Hack to flip
	auto eye1 = frameIn.Tracking.Eye[1];
	frameIn.Tracking.Eye[1] = frameIn.Tracking.Eye[0];
	frameIn.Tracking.Eye[0] = eye1;
#endif

	return AppFrame( frameIn );
}

void ovrAppl::RenderFrame( const ovrApplFrameIn & in )
{
	ovrRendererOutput out = {};
	// default the Projection for each eye to whatever the input frame has, 
	// but let the application override this
	out.FrameMatrices.EyeProjection[0] = in.Tracking.Eye[0].ProjectionMatrix;
	out.FrameMatrices.EyeProjection[1] = in.Tracking.Eye[1].ProjectionMatrix;

	// Check for "Back" presses
	bool backButtonPressed = false;
	std::vector<ovrButton> backButtons = { ovrButton_B, ovrButton_Y, ovrButton_Back };
	for ( const auto & button : backButtons )
	{
		if ( in.Clicked( button ) )
		{
			backButtonPressed = true;
			break;
		}
	}

	if ( backButtonPressed )
	{
		AppHandleInputShutdownRequest( out );
	}
	else
	{
		AppRenderFrame( in, out );
	}


	const ovrLayerHeader2 * layerPtrs[ovrMaxLayerCount] = {};
	for ( int i = 0; i < out.NumLayers; ++i )
	{
		layerPtrs[i] = &out.Layers[i].Header;
	}

	ovrSubmitFrameDescription2 frameDesc = {};
	frameDesc.Flags = out.FrameFlags;
	frameDesc.SwapInterval = 1;
	frameDesc.FrameIndex = GetFrameIndex();
	frameDesc.DisplayTime = GetDisplayTime();
	frameDesc.LayerCount = out.NumLayers;
	frameDesc.Layers = layerPtrs;

	vrapi_SubmitFrame2( GetSessionObject(), &frameDesc );	
}

ovrApplFrameOut ovrAppl::AppFrame( const ovrApplFrameIn & /* in */ )
{
	ALOGV( "Default AppFrame called!" );
	return ovrApplFrameOut( false );
}

void ovrAppl::AppRenderFrame( const ovrApplFrameIn & in, ovrRendererOutput & out )
{
	ALOGV( "Default AppRenderFrame called!" );
}

void ovrAppl::AppRenderEye( const ovrApplFrameIn & /* in */, ovrRendererOutput & /* out */, int /* eye */ )
{
}

void ovrAppl::AppEyeGLStateSetup( const ovrApplFrameIn & /* in */, const ovrFramebuffer * fb, int /* eye */ )
{
	GL( glEnable( GL_SCISSOR_TEST ) );
	GL( glDepthMask( GL_TRUE ) );
	GL( glEnable( GL_DEPTH_TEST ) );
	GL( glDepthFunc( GL_LEQUAL ) );
	GL( glEnable( GL_CULL_FACE ) );
	GL( glViewport( 0, 0, fb->Width, fb->Height ) );
	GL( glScissor( 0, 0, fb->Width, fb->Height ) );
	GL( glClearColor( 0.125f, 0.0f, 0.125f, 1.0f ) );
	GL( glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT ) );
}

void ovrAppl::DefaultRenderFrame_Loading( const ovrApplFrameIn & in, ovrRendererOutput & out )
{
	// black layer
	ovrLayerProjection2 blackLayer = vrapi_DefaultLayerBlackProjection2();
	blackLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;
	out.Layers[out.NumLayers++].Projection = blackLayer;
	// loading icon layer
	ovrLayerLoadingIcon2 iconLayer = vrapi_DefaultLayerLoadingIcon2();
	iconLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;
	out.Layers[out.NumLayers++].LoadingIcon = iconLayer;
	out.FrameFlags |= VRAPI_FRAME_FLAG_FLUSH;
}

void ovrAppl::DefaultRenderFrame_Running( const ovrApplFrameIn & in, ovrRendererOutput & out )
{
	// set up layers
	int & layerCount = out.NumLayers;
	layerCount = 0;
	ovrLayerProjection2 & layer = out.Layers[layerCount].Projection;
	layer = vrapi_DefaultLayerProjection2();
	layer.HeadPose = in.Tracking.HeadPose;
	for ( int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; ++eye )
	{
		ovrFramebuffer * framebuffer = Framebuffer[NumFramebuffers == 1 ? 0 : eye ].get();
		layer.Textures[eye].ColorSwapChain = framebuffer->ColorTextureSwapChain;
		layer.Textures[eye].SwapChainIndex = framebuffer->TextureSwapChainIndex;
		layer.Textures[eye].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection( (ovrMatrix4f *)&out.FrameMatrices.EyeProjection[eye] );
	}
	layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;
	layerCount++;

	// render images for each eye
	for ( int eye = 0; eye < NumFramebuffers; ++eye )
	{
		ovrFramebuffer * framebuffer = Framebuffer[eye].get();
		ovrFramebuffer_SetCurrent( framebuffer );

		AppEyeGLStateSetup( in, framebuffer, eye );
		AppRenderEye( in, out, eye );

		ovrFramebuffer_Resolve( framebuffer );
		ovrFramebuffer_Advance( framebuffer );
	}

	ovrFramebuffer_SetNone();
}

void ovrAppl::DefaultRenderFrame_Ending( const ovrApplFrameIn & in, ovrRendererOutput & out )
{
	ovrLayerProjection2 layer = vrapi_DefaultLayerBlackProjection2();
	layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;
	out.Layers[out.NumLayers++].Projection = layer;
	out.FrameFlags |= VRAPI_FRAME_FLAG_FLUSH | VRAPI_FRAME_FLAG_FINAL;
}


void ovrAppl::AddKeyEvent( const int32_t keyCode, const int32_t action )
{
	// lock while we push back the event because we could be copying events into
	// the event buffer when the OS calls back the input handler
	std::lock_guard< std::mutex > lock( KeyEventMutex );
	PendingKeyEvents.push_back( ovrKeyEvent( keyCode, action, GetTimeInSeconds() ) );
}

void ovrAppl::AddTouchEvent( const int32_t action, const int32_t x, const int32_t y )
{
	// lock while we push back the event because we could be copying events into
	// the event buffer when the OS calls back the input handler
	std::lock_guard< std::mutex > lock( TouchEventMutex );
	PendingTouchEvents.push_back( ovrTouchEvent( action, x, y, GetTimeInSeconds() ) );
}

void ovrAppl::HandleVREvents( ovrApplFrameIn & in )
{
	in.LeftRemoteTracked = false;
	in.RightRemoteTracked = false;
	in.SingleHandRemoteTracked = false;
	in.GamePadTracked = false;
	in.HeadsetInputTracked = false;
	in.SingleHandRemoteTrackpadMaxX = 0u;
	in.SingleHandRemoteTrackpadMaxY = 0u;							
	in.HeadsetTrackpadMaxX = 0u;
	in.HeadsetTrackpadMaxY = 0u;							
	in.AllButtons = 0u;
	in.AllTouches = 0u;

	const ovrJava * java = reinterpret_cast< const ovrJava* >( Context->ContextForVrApi() );
	bool trackHeadset = false;
	ovrDeviceType deviceType = ( ovrDeviceType ) vrapi_GetSystemPropertyInt( java, VRAPI_SYS_PROP_DEVICE_TYPE );
	if ( deviceType >= VRAPI_DEVICE_TYPE_GEARVR_START && deviceType <= VRAPI_DEVICE_TYPE_GEARVR_END )
	{
		trackHeadset = true;
	}

	// Track mount status
	in.HeadsetIsMounted = ( vrapi_GetSystemStatusInt( java, VRAPI_SYS_STATUS_MOUNTED ) != VRAPI_FALSE );

	// Enumerate input devices
	for ( ovrDeviceID deviceIndex = 0; ; deviceIndex++ )
	{
		ovrInputCapabilityHeader capsHeader;

		if ( vrapi_EnumerateInputDevices( GetSessionObject(), deviceIndex, &capsHeader ) < 0 )
		{
			break;	// no more devices
		}

		/// deviceID is not the same as deviceIndex!
		ovrDeviceID deviceID = capsHeader.DeviceID;
		if ( deviceID == ovrDeviceIdType_Invalid )
		{
			ALOGV( "HandleVREvents - Invalid deviceID for deviceIndex=%u", deviceIndex );
			assert( deviceID != ovrDeviceIdType_Invalid );
			continue;
		}

		// Focus on remotes for now
		if ( capsHeader.Type == ovrControllerType_TrackedRemote ) 
		{
			ovrInputTrackedRemoteCapabilities remoteCaps;
			remoteCaps.Header = capsHeader;
			if ( ovrSuccess == vrapi_GetInputDeviceCapabilities( GetSessionObject(), &remoteCaps.Header ) )
			{
				bool isLeft = (remoteCaps.ControllerCapabilities & ovrControllerCaps_LeftHand) != 0;
				bool isRight = (remoteCaps.ControllerCapabilities & ovrControllerCaps_RightHand) != 0;
				/// allow for single-handed controlles
				bool isGearVR = (remoteCaps.ControllerCapabilities & ovrControllerCaps_ModelGearVR) != 0;
				bool isOculusGO = (remoteCaps.ControllerCapabilities & ovrControllerCaps_ModelOculusGo) != 0;
				bool isSingleHandRemote = isGearVR || isOculusGO;

				ovrInputStateTrackedRemote state;
				state.Header.ControllerType = ovrControllerType_TrackedRemote;

    			if ( ovrSuccess == vrapi_GetCurrentInputState( GetSessionObject(), deviceID, &state.Header ) ) 
    			{
    				if ( isLeft && !isSingleHandRemote )
					{
						in.LeftRemoteTracked = true;
						in.LeftRemote = state;
					}

    				if ( isRight && !isSingleHandRemote )
					{
						in.RightRemoteTracked = true;
						in.RightRemote = state;
					}

					if ( isSingleHandRemote )
					{
						in.SingleHandRemoteTracked = true;
						in.SingleHandRemote = state;
						in.SingleHandRemoteTrackpadMaxX = remoteCaps.TrackpadMaxX;
						in.SingleHandRemoteTrackpadMaxY = remoteCaps.TrackpadMaxY;							
					}
    			}
			}
		}
		else if ( capsHeader.Type == ovrControllerType_Gamepad )
		{
			ovrInputStateGamepad state;
			state.Header.ControllerType = ovrControllerType_Gamepad;
			if ( ovrSuccess == vrapi_GetCurrentInputState( GetSessionObject(), deviceID, &state.Header ) ) 
			{
				in.GamePadTracked = true;
				in.GamePad = state;
			}
		}
		else if ( capsHeader.Type == ovrControllerType_Headset && trackHeadset )
		{
			ovrInputStateHeadset state;
			state.Header.ControllerType = ovrControllerType_Headset;
			if ( ovrSuccess == vrapi_GetCurrentInputState( GetSessionObject(), deviceID, &state.Header ) ) 
			{
				ovrInputHeadsetCapabilities headsetCaps;
				headsetCaps.Header = capsHeader;
				if ( ovrSuccess == vrapi_GetInputDeviceCapabilities( GetSessionObject(), &headsetCaps.Header ) )
				{
					in.HeadsetInputTracked = true;
					in.Headset = state;
					in.HeadsetTrackpadMaxX = headsetCaps.TrackpadMaxX;
					in.HeadsetTrackpadMaxY = headsetCaps.TrackpadMaxY;							
				}

			}
		}
	}

	// Aggregate button events
	if ( in.LeftRemoteTracked )
	{
		in.AllButtons |= in.LeftRemote.Buttons;
		in.AllTouches |= in.LeftRemote.Touches;
	}
	if ( in.RightRemoteTracked )
	{
		in.AllButtons |= in.RightRemote.Buttons;
		in.AllTouches |= in.RightRemote.Touches;
	}
	if ( in.SingleHandRemoteTracked )
	{
		in.AllButtons |= in.SingleHandRemote.Buttons;
		in.AllTouches |= in.SingleHandRemote.Touches;
	}
	if ( in.GamePadTracked )
	{
		in.AllButtons |= in.GamePad.Buttons;
	}
	if ( in.HeadsetInputTracked )
	{
		in.AllButtons |= in.Headset.Buttons;
	}

	// Delta from last frame
	in.LastFrameAllButtons = LastFrameAllButtons;
	in.LastFrameAllTouches = LastFrameAllTouches;
	in.LastFrameHeadsetIsMounted = LastFrameHeadsetIsMounted;
	LastFrameAllButtons = in.AllButtons;
	LastFrameAllTouches = in.AllTouches;
	LastFrameHeadsetIsMounted = in.HeadsetIsMounted;
}

void ovrAppl::Run( struct android_app * app )
{
	ALOGV( "----------------------------------------------------------------" );
	ALOGV( "android_main()" );
	ALOGV( "----------------------------------------------------------------" );

	ANativeActivity_setWindowFlags( app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0 );

	std::unique_ptr< OVRFW::ovrAndroidContext > ctx = std::unique_ptr< OVRFW::ovrAndroidContext >( new OVRFW::ovrAndroidContext() );
	ctx->Init( app->activity->vm, app->activity->clazz, "main" );

	if ( !Init( ctx.get() ) )
	{
		exit( EXIT_FAILURE );
	}

	app->userData = this;
	app->onAppCmd = android_handle_cmd;
	app->onInputEvent = android_handle_input;

	bool exitApp = false;

	// main loop
	while( app->destroyRequested == 0 )
	{
		for ( ;; )
		{
			int events;
			struct android_poll_source * source = nullptr;
			// if we are not in VR mode and not waiting on the app to end, wait indefinitely in the poll (-1),
			// otherwise, return immediately
			const int timeoutMilliseconds = ( !HasActiveVRSession() && app->destroyRequested == 0 ) ? -1 : 0;
			if ( ALooper_pollAll( timeoutMilliseconds, nullptr, &events, (void**)&source ) < 0 )
			{
				break;
			}
			if ( source != nullptr )
			{
				source->process( app, source );
			}

			HandleLifecycle( ctx.get() );
		}

		if ( !HasActiveVRSession() )
		{
			continue;
		}

		OVRFW::ovrApplFrameIn frameIn;
		OVRFW::ovrApplFrameOut frameOut = Frame( frameIn );
		RenderFrame( frameIn );

		exitApp = frameOut.ExitApp;
	}

	Shutdown( ctx.get() );
	ctx->Shutdown();

	ALOGV( "----------------------------------------------------------------" );
	ALOGV( "android_main() DONE" );
	ALOGV( "----------------------------------------------------------------" );
}

} // namespace OVRFW
