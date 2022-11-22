/************************************************************************************

Filename    :   Native.cpp
Content     :
Created     :	6/20/2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include <VrApi_Types.h>
#include "CinemaApp.h"
#include "Native.h"
#include "System.h"
#include <JniUtils.h>
#include <android_native_app_glue.h>

using namespace OVRFW;

#if defined( OVR_OS_ANDROID )
extern "C" {

OculusCinema::CinemaApp * appPtr = NULL;

long Java_com_oculus_cinemasdk_MainActivity_nativeSetAppInterface( JNIEnv *jni, jclass clazz, jobject activity)
{
	ALOG( "nativeSetAppInterface %p", appPtr );
	return reinterpret_cast<jlong>( appPtr );
}

void Java_com_oculus_cinemasdk_MainActivity_nativeSetVideoSize( JNIEnv *jni, jclass clazz, jlong interfacePtr, int width, int height)
{
	OVR_LOG( "nativeSetVideoSizes: width=%i height=%i", width, height);

	OculusCinema::CinemaApp * cinema = appPtr;//static_cast< OculusCinema::CinemaApp * >( ( (App *)interfacePtr )->GetAppInterface() );
	cinema->GetMessageQueue().PostPrintf( "video %i %i", width, height);

}

jobject Java_com_oculus_cinemasdk_MainActivity_nativePrepareNewVideo( JNIEnv *jni, jclass clazz, jlong interfacePtr )
{
	OculusCinema::CinemaApp * cinema = appPtr;//static_cast< OculusCinema::CinemaApp * >( ( (App *)interfacePtr )->GetAppInterface() );

	// set up a message queue to get the return message
	// TODO: make a class that encapsulates this work
	ovrMessageQueue result( 1 );
	cinema->GetMessageQueue().PostPrintf( "newVideo %p", &result );

	result.SleepUntilMessage();
	const char * msg = result.GetNextMessage();
	jobject	texobj;
	sscanf( msg, "surfaceTexture %p", &texobj );
	free( (void *)msg );

	return texobj;

}

void Java_com_oculus_cinemasdk_MainActivity_nativeDisplayMessage( JNIEnv *jni, jclass clazz, jlong interfacePtr, jstring text, int time, bool isError ) {}
void Java_com_oculus_cinemasdk_MainActivity_nativeAddPc( JNIEnv *jni, jclass clazz, jlong interfacePtr, jstring name, jstring uuid, int psi, int reach, jstring binding, bool isRunning)
{
    OculusCinema::CinemaApp *cinema =  appPtr;//( OculusCinema::CinemaApp * )( ( (App *)interfacePtr )->GetAppInterface() );
    JavaUTFChars utfName( jni, name );
    JavaUTFChars utfUUID( jni, uuid );
    JavaUTFChars utfBind( jni, binding );

    OculusCinema::Native::PairState ps = (OculusCinema::Native::PairState) psi;
    //cinema->PcMgr.AddPc(utfName.ToStr(), utfUUID.ToStr(), ps, utfBind.ToStr(), width, height);
	OculusCinema::Native::Reachability rs = (OculusCinema::Native::Reachability) reach;
	cinema->PcMgr.AddPc(utfName.ToStr(), utfUUID.ToStr(), ps, rs, utfBind.ToStr(), isRunning);

}
void Java_com_oculus_cinemasdk_MainActivity_nativeRemovePc( JNIEnv *jni, jclass clazz, jlong interfacePtr, jstring name)
{
    OculusCinema::CinemaApp *cinema =  appPtr;//( OculusCinema::CinemaApp * )( ( (App *)interfacePtr )->GetAppInterface() );
    JavaUTFChars utfName( jni, name );
    cinema->PcMgr.RemovePc(utfName.ToStr());
}
void Java_com_oculus_cinemasdk_MainActivity_nativeAddApp( JNIEnv *jni, jclass clazz, jlong interfacePtr, jstring name, jstring posterfilename, int id, bool isRunning)
{

    OculusCinema::CinemaApp *cinema =  appPtr;//( OculusCinema::CinemaApp * )( ( (App *)interfacePtr )->GetAppInterface() );
    JavaUTFChars utfName( jni, name );
    JavaUTFChars utfPosterFileName( jni, posterfilename );
    cinema->AppMgr.AddApp(utfName.ToStr(), utfPosterFileName.ToStr(), id, isRunning);

}
void Java_com_oculus_cinemasdk_MainActivity_nativeRemoveApp( JNIEnv *jni, jclass clazz, jlong interfacePtr, int id)
{

    OculusCinema::CinemaApp *cinema =  appPtr;//( OculusCinema::CinemaApp * )( ( (App *)interfacePtr )->GetAppInterface() );
    cinema->AppMgr.RemoveApp( id);

}


void Java_com_oculus_cinemasdk_MainActivity_nativeShowPair( JNIEnv *jni, jclass clazz, jlong interfacePtr, jstring message )
{

    OculusCinema::CinemaApp *cinema =  appPtr;//( OculusCinema::CinemaApp * )( ( (App *)interfacePtr )->GetAppInterface() );
    JavaUTFChars utfMessage( jni, message );
    cinema->ShowPair(utfMessage.ToStr());

}
void Java_com_oculus_cinemasdk_MainActivity_nativePairSuccess( JNIEnv *jni, jclass clazz, jlong interfacePtr )
{

    OculusCinema::CinemaApp *cinema =  appPtr;//( OculusCinema::CinemaApp * )( ( (App *)interfacePtr )->GetAppInterface() );
    cinema->PairSuccess();

}
void Java_com_oculus_cinemasdk_MainActivity_nativeShowError( JNIEnv *jni, jclass clazz, jlong interfacePtr, jstring message )
{

    OculusCinema::CinemaApp *cinema =  appPtr;//( OculusCinema::CinemaApp * )( ( (App *)interfacePtr )->GetAppInterface() );
    JavaUTFChars utfMessage( jni, message );
    cinema->ShowError(utfMessage.ToStr());

}
void Java_com_oculus_cinemasdk_MainActivity_nativeClearError( JNIEnv *jni, jclass clazz, jlong interfacePtr )
{

    OculusCinema::CinemaApp *cinema =  appPtr;//( OculusCinema::CinemaApp * )( ( (App *)interfacePtr )->GetAppInterface() );
    cinema->ClearError();

}

}	// extern "C"
#endif


//==============================================================
// android_main
//==============================================================
void android_main( struct android_app * app )
{
	std::unique_ptr< OculusCinema::CinemaApp > appl = std::unique_ptr< OculusCinema::CinemaApp >( new OculusCinema::CinemaApp( 0, 0, 0, 0 ) );
	appPtr = appl.get();
	appl->Run( app );
}

//==============================================================

namespace OculusCinema
{

// Java method ids
static jmethodID 	getExternalCacheDirectoryMethodId = NULL;
static jmethodID	createVideoThumbnailMethodId = NULL;
static jmethodID 	isPlayingMethodId = NULL;
static jmethodID 	isPlaybackFinishedMethodId = NULL;
static jmethodID 	hadPlaybackErrorMethodId = NULL;
static jmethodID 	startMovieMethodId = NULL;
static jmethodID 	stopMovieMethodId = NULL;
static jmethodID     initPcSelectorMethodId = NULL;
static jmethodID     pairPcMethodId = NULL;
static jmethodID     getPcPairStateMethodId = NULL;
static jmethodID     getPcStateMethodId = NULL;
static jmethodID     getPcReachabilityMethodId = NULL;
static jmethodID    addPCbyIPMethodId = NULL;
static jmethodID     initAppSelectorMethodId = NULL;
static jmethodID    mouseMoveMethodId = NULL;
static jmethodID    mouseClickMethodId = NULL;
static jmethodID    mouseScrollMethodId = NULL;
static jmethodID    getLastFrameTimestampMethodId = NULL;
static jmethodID    currentTimeMillisMethodId = NULL;
static jmethodID    closeAppMethodId = NULL;




#if defined( OVR_OS_ANDROID )
// Error checks and exits on failure
static jmethodID GetMethodID( jclass cls, const char * name, const char * signature )
{
    const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	JNIEnv *env;
	ctx.Vm->AttachCurrentThread(&env, 0);
	jmethodID mid = env->GetMethodID( cls, name, signature );
	if ( !mid )
	{
    	OVR_FAIL( "Couldn't find %s methodID", name );
    }

	return mid;
}
#endif

void Native::OneTimeInit( jclass mainActivityClass )
{
	OVR_LOG( "Native::OneTimeInit" );

	const double start = GetTimeInSeconds();

#if defined( OVR_OS_ANDROID )
	getExternalCacheDirectoryMethodId 	= GetMethodID(  mainActivityClass, "getExternalCacheDirectory", "()Ljava/lang/String;" );
	createVideoThumbnailMethodId        = GetMethodID(  mainActivityClass, "createVideoThumbnail", "(Ljava/lang/String;ILjava/lang/String;II)Z" );
	isPlayingMethodId 					= GetMethodID(  mainActivityClass, "isPlaying", "()Z" );
	isPlaybackFinishedMethodId			= GetMethodID(  mainActivityClass, "isPlaybackFinished", "()Z" );
	hadPlaybackErrorMethodId			= GetMethodID(  mainActivityClass, "hadPlaybackError", "()Z" );
	startMovieMethodId                     = GetMethodID(  mainActivityClass, "startMovie", "(Ljava/lang/String;Ljava/lang/String;ILjava/lang/String;IIIZIZ)V" );
	stopMovieMethodId 					= GetMethodID(  mainActivityClass, "stopMovie", "()V" );
	initPcSelectorMethodId                 = GetMethodID(  mainActivityClass, "initPcSelector", "()V" );
    pairPcMethodId                         = GetMethodID(  mainActivityClass, "pairPc", "(Ljava/lang/String;)V" );
    getPcPairStateMethodId                 = GetMethodID(  mainActivityClass, "getPcPairState", "(Ljava/lang/String;)I" );
    getPcStateMethodId                     = GetMethodID(  mainActivityClass, "getPcState", "(Ljava/lang/String;)I" );
    getPcReachabilityMethodId             = GetMethodID(  mainActivityClass, "getPcReachability", "(Ljava/lang/String;)I" );
	addPCbyIPMethodId                    = GetMethodID(  mainActivityClass, "addPCbyIP", "(Ljava/lang/String;)I" );
	initAppSelectorMethodId             = GetMethodID(  mainActivityClass, "initAppSelector", "(Ljava/lang/String;)V" );
    mouseMoveMethodId                     = GetMethodID(  mainActivityClass, "mouseMove", "(II)V" );
    mouseClickMethodId                     = GetMethodID(  mainActivityClass, "mouseClick", "(IZ)V" );
    mouseScrollMethodId                 = GetMethodID(  mainActivityClass, "mouseScroll", "(B)V" );
	getLastFrameTimestampMethodId        = GetMethodID(  mainActivityClass, "getLastFrameTimestamp", "()J" );
	currentTimeMillisMethodId            = GetMethodID(  mainActivityClass, "currentTimeMillis", "()J" );
	closeAppMethodId                    = GetMethodID(  mainActivityClass, "closeApp", "(Ljava/lang/String;I)V" );



#endif
	OVR_LOG( "Native::OneTimeInit: %3.1f seconds", GetTimeInSeconds() - start );
}

void Native::OneTimeShutdown()
{
	OVR_LOG( "Native::OneTimeShutdown" );
}

std::string Native::GetExternalCacheDirectory()
{
#if defined( OVR_OS_ANDROID )
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	jstring externalCacheDirectoryString = (jstring)ctx.Env->CallObjectMethod( ctx.ActivityObject, getExternalCacheDirectoryMethodId );

	const char *externalCacheDirectoryStringUTFChars = ctx.Env->GetStringUTFChars( externalCacheDirectoryString, NULL );
	std::string externalCacheDirectory = externalCacheDirectoryStringUTFChars;

	ctx.Env->ReleaseStringUTFChars( externalCacheDirectoryString, externalCacheDirectoryStringUTFChars );
	ctx.Env->DeleteLocalRef( externalCacheDirectoryString );

	return externalCacheDirectory;
#else
	return std::string();
#endif
}

bool Native::CreateVideoThumbnail( const char *uuid, int appId, const char *outputFilePath, const int width, const int height )
{
	OVR_LOG( "CreateVideoThumbnail( %s, %i, %s )", uuid, appId, outputFilePath );
#if defined( OVR_OS_ANDROID )
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	jstring jstrUUID = ctx.Env->NewStringUTF( uuid );

	jstring jstrOutputFilePath = ctx.Env->NewStringUTF( outputFilePath );


    //todo rafa

    jboolean result = ctx.Env->CallBooleanMethod( ctx.ActivityObject, createVideoThumbnailMethodId, jstrUUID, appId, jstrOutputFilePath, width, height );
    OVR_LOG( "Done creating thumbnail!");
    ctx.Env->DeleteLocalRef( jstrUUID );

	ctx.Env->DeleteLocalRef( jstrOutputFilePath );

	//OVR_LOG( "CreateVideoThumbnail( %s, %s )", videoFilePath, outputFilePath );

	return result;
#else
	return false;
#endif
}

bool Native::IsPlaying()
{
	OVR_LOG( "IsPlaying()" );
#if defined( OVR_OS_ANDROID )
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	return ctx.Env->CallBooleanMethod( ctx.ActivityObject, isPlayingMethodId );
#else
	return false;
#endif
}

bool Native::IsPlaybackFinished(  )
{
#if defined( OVR_OS_ANDROID )
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	jboolean result = ctx.Env->CallBooleanMethod( ctx.ActivityObject, isPlaybackFinishedMethodId );
	return ( result != 0 );
#else
	return false;
#endif
}

bool Native::HadPlaybackError(  )
{
#if defined( OVR_OS_ANDROID )
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	jboolean result = ctx.Env->CallBooleanMethod( ctx.ActivityObject, hadPlaybackErrorMethodId );
	return ( result != 0 );
#else
	return false;
#endif
}

void Native::StartMovie(  const char * uuid, const char * appName, int id, const char * binder, int width, int height, int fps, bool hostAudio, int customBitrate, bool remote )

{
	OVR_LOG( "StartMovie( %s )", appName );
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	jstring jstrUUID = ctx.Env->NewStringUTF( uuid );
	jstring jstrAppName = ctx.Env->NewStringUTF( appName );
	jstring jstrBinder = ctx.Env->NewStringUTF( binder );


	ctx.Env->CallVoidMethod( ctx.ActivityObject, startMovieMethodId, jstrUUID, jstrAppName, id, jstrBinder, width, height, fps, hostAudio, customBitrate, remote );

	ctx.Env->DeleteLocalRef( jstrUUID );
	ctx.Env->DeleteLocalRef( jstrAppName );
	ctx.Env->DeleteLocalRef( jstrBinder );
}

void Native::StopMovie( )
{
	OVR_LOG( "StopMovie()" );
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	ctx.Env->CallVoidMethod( ctx.ActivityObject, stopMovieMethodId );
}

void Native::InitPcSelector( )
{
	OVR_LOG( "InitPcSelector()" );
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	ctx.Env->CallVoidMethod( ctx.ActivityObject, initPcSelectorMethodId );
}

void Native::InitAppSelector( const char* uuid)
{
	OVR_LOG( "InitAppSelector()" );
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	jstring jstrUUID = ctx.Env->NewStringUTF( uuid );
	ctx.Env->CallVoidMethod( ctx.ActivityObject, initAppSelectorMethodId, jstrUUID );
}

Native::PairState Native::GetPairState(  const char* uuid)
{
	OVR_LOG( "GetPairState()" );
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	jstring jstrUUID = ctx.Env->NewStringUTF( uuid );
	return (PairState)ctx.Env->CallIntMethod( ctx.ActivityObject, getPcPairStateMethodId, jstrUUID );
}

void Native::Pair( const char* uuid)
{
	OVR_LOG( "Pair()" );
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	jstring jstrUUID = ctx.Env->NewStringUTF( uuid );
	ctx.Env->CallVoidMethod( ctx.ActivityObject, pairPcMethodId, jstrUUID );
}

void Native::MouseMove(int deltaX, int deltaY)
{
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	ctx.Env->CallVoidMethod( ctx.ActivityObject, mouseMoveMethodId, deltaX, deltaY );
}

void Native::MouseClick( int buttonId, bool down)
{
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	ctx.Env->CallVoidMethod( ctx.ActivityObject, mouseClickMethodId, buttonId, down );
}

void Native::MouseScroll( signed char amount)
{
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	ctx.Env->CallVoidMethod( ctx.ActivityObject, mouseScrollMethodId, amount );
}

void Native::closeApp( const char* uuid, int appID)
{
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	jstring jstrUUID = ctx.Env->NewStringUTF( uuid );
	ctx.Env->CallVoidMethod( ctx.ActivityObject, closeAppMethodId, jstrUUID, appID );
	ctx.Env->DeleteLocalRef( jstrUUID );
}

long Native::getLastFrameTimestamp()
{
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	return ctx.Env->CallLongMethod( ctx.ActivityObject, getLastFrameTimestampMethodId );
}
long Native::currentTimeMillis()
{
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	return ctx.Env->CallLongMethod( ctx.ActivityObject, currentTimeMillisMethodId );
}


int Native::addPCbyIP( const char* ip)
{
	const ovrJava & ctx = *( reinterpret_cast< const ovrJava* >( appPtr->GetContext()->ContextForVrApi() ) );
	jstring jstrIP = ctx.Env->NewStringUTF( ip );
	int result = ctx.Env->CallIntMethod( ctx.ActivityObject, addPCbyIPMethodId, jstrIP );
	ctx.Env->DeleteLocalRef( jstrIP );
	return result;
}



} // namespace OculusCinema
