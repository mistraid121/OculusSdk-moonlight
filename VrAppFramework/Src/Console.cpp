/************************************************************************************

Filename    :   Console.cpp
Content     :   Allows adb to send commands to an application.
Created     :   11/21/2014
Authors     :   Jonathan Wright

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#include "Console.h"

#include <vector>

#include "JniUtils.h"
#include "OVR_LogUtils.h"

//#define ALLOW_RELEASE_CONSOLE

namespace OVR {

class OvrConsole
{
public:
	void RegisterConsoleFunction( const char * name, consoleFn_t inFunction )
	{
		//OVR_LOG( "Registering console function '%s'", name );
		for ( const OvrConsoleFunction & functions : ConsoleFunctions )
		{
			if ( OVR_stricmp( functions.GetName(), name ) == 0 )
			{
				OVR_LOG_WITH_TAG( "OvrConsole", "Console function '%s' is already registered!!", name );
				OVR_ASSERT( false );	// why are you registering the same function twice??
				return;
			}
		}
		OVR_LOG( "Registered console function '%s'", name );
		ConsoleFunctions.push_back( OvrConsoleFunction( name, inFunction ) );
	}

	void UnRegisterConsoleFunctions()
	{
		ConsoleFunctions.clear();
	}

	void ExecuteConsoleFunction( intptr_t appPtr, char const * commandStr ) const
	{
		OVR_LOG_WITH_TAG( "OvrConsole", "Received console command \"%s\"", commandStr );
	
		char cmdName[128];
		char const * parms = "";
		int cmdLen = (int)strlen( commandStr );
		char const * spacePtr = strstr( commandStr, " " );
		if ( spacePtr != NULL && spacePtr - commandStr < cmdLen )
		{
			parms = spacePtr + 1;
			OVR_strncpy( cmdName, sizeof( cmdName ), commandStr, spacePtr - commandStr );
		} 
		else
		{
			OVR_strcpy( cmdName, sizeof( cmdName ), commandStr );
		}

		OVR_LOG( "ExecuteConsoleFunction( %s, %s )", cmdName, parms );
		for ( const OvrConsoleFunction & function : ConsoleFunctions )
		{
			OVR_LOG( "Checking console function '%s'", function.GetName() );
			if ( OVR_stricmp( function.GetName(), cmdName ) == 0 )
			{
				OVR_LOG( "Executing console function '%s'", cmdName );
				function.Execute( reinterpret_cast< void* >( appPtr ), parms );
				return;
			}
		}

		OVR_LOG_WITH_TAG( "OvrConsole", "ERROR: unknown console command '%s'", cmdName );
	}

	void GetConsoleCmds( void( *callback )( const char * cmd ) )
	{
		for ( const OvrConsoleFunction & function : ConsoleFunctions )
		{
			callback( function.GetName() );
		}
	}

private:
	class OvrConsoleFunction
	{
	public:
		OvrConsoleFunction( const char * name, consoleFn_t function ) :
			Function( function )
		{
			OVR::OVR_strcpy( Name, sizeof( Name ), name );
		}

		const char *	GetName() const { return Name; }
		void			Execute( void * appPtr, const char * cmd ) const { Function( appPtr, cmd ); }

	private:
		char			Name[64];		// not an OVR::String because this can be freed after the OVR heap has been destroyed.
		consoleFn_t		Function;
	};

	std::vector< OvrConsoleFunction >	ConsoleFunctions;
};

OvrConsole * Console = NULL;

void InitConsole( ovrJava & java )
{
	Console = new OvrConsole;

#if defined( OVR_OS_ANDROID )
	// start the console receiver
	{
		jclass consoleReceiverClass = ovr_GetGlobalClassReference( java.Env, java.ActivityObject, "com/oculus/vrappframework/ConsoleReceiver" );
		const jmethodID startConsoleReceiverId = ovr_GetStaticMethodID( java.Env, consoleReceiverClass,
			"startReceiver", "(Landroid/app/Activity;)V" );
		java.Env->CallStaticVoidMethod( consoleReceiverClass, startConsoleReceiverId, java.ActivityObject );

		java.Env->DeleteGlobalRef( consoleReceiverClass );
	}
#else
	OVR_UNUSED( java );
#endif
}

void ShutdownConsole( ovrJava & java )
{
#if defined( OVR_OS_ANDROID )
	// Unregister the console receiver
	{
		JavaClass javaConsoleReceiverClass( java.Env, ovr_GetLocalClassReference( java.Env, java.ActivityObject, "com/oculus/vrappframework/ConsoleReceiver" ) );
		const jmethodID stopReceiverMethodId = ovr_GetStaticMethodID( java.Env, javaConsoleReceiverClass.GetJClass(), "stopReceiver", "(Landroid/app/Activity;)V" );
		if ( stopReceiverMethodId != NULL )
		{
			java.Env->CallStaticVoidMethod( javaConsoleReceiverClass.GetJClass(), stopReceiverMethodId, java.ActivityObject );
		}
	}
#endif	

	if ( Console != NULL )
	{
		Console->UnRegisterConsoleFunctions();
	}
	delete Console;
	Console = NULL;
}

// add a pointer to a function that can be executed from the console
void RegisterConsoleFunction( const char * name, consoleFn_t function )
{
	Console->RegisterConsoleFunction( name, function );
}

void DebugPrint( void * appPtr, const char * cmd )
{
	OVR_UNUSED( appPtr );
	OVR_LOG_WITH_TAG( "OvrDebug", "%s", cmd );
}

void SendConsoleCmd( void * appPtr, const char * cmd )
{
	if ( OVR::Console != NULL )
	{
		OVR::Console->ExecuteConsoleFunction( (intptr_t)appPtr, cmd );
	}
}	

void GetConsoleCmds( void( *callback )( const char * cmd ) )
{
	if ( OVR::Console != NULL )
	{
		OVR::Console->GetConsoleCmds( callback );
	}
}

}	// namespace Ovr

#if defined( OVR_OS_ANDROID )
extern "C" {

JNIEXPORT void Java_com_oculus_vrappframework_ConsoleReceiver_nativeConsoleCommand( JNIEnv * jni, jclass clazz, jlong appPtr, jstring command )
{
#if defined( ALLOW_RELEASE_CONSOLE ) || defined( OVR_BUILD_DEBUG )
	if ( command == NULL )
	{
		return;
	}
	char const * commandStr = ovr_GetStringUTFChars( jni, command, NULL );
	OVR_LOG( "nativeConsoleCommand: %s", commandStr );
	if ( OVR::Console != NULL )
	{
		OVR::Console->ExecuteConsoleFunction( appPtr, commandStr );
	}
	else
	{
		OVR_LOG( "Tried to execute console function without a console!" );
	}
	jni->ReleaseStringUTFChars( command, commandStr );
#endif	
}

}
#endif
