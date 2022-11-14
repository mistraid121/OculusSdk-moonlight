/************************************************************************************

Filename    :   Native.h
Content     :
Created     :	8/8/2014
Authors     :   Jim Dosé

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( Native_h )
#define Native_h

#include <string>

#include "Appl.h"


namespace OculusCinema {

class Native {
public:
	static void			OneTimeInit(jclass mainActivityClass );
	static void			OneTimeShutdown();

	static std::string	GetExternalCacheDirectory();  	// returns path to app specific writable directory
	static bool 		CreateVideoThumbnail( const char *uuid, int appId, const char *outputFilePath, const int width, const int height );

	static bool			IsPlaying();
	static bool 		IsPlaybackFinished();
	static bool 		HadPlaybackError();

	static void         StartMovie(const char * uuid, const char * appName, int id, const char * binder, int width, int height, int fps, bool hostAudio, int customBitrate, bool remote );


	static void 		StopMovie();
	
    enum PairState {
        NOT_PAIRED = 0,
        PAIRED,
        PIN_WRONG,
        FAILED
    };

    enum CompState {
        ONLINE = 0,
        OFFLINE,
        UNKNOWN_STATE
    };

    enum Reachability {
        LOCAL = 0,
        REMOTE,
        RS_OFFLINE,
        UNKNOWN_REACH
    };

    static void            InitPcSelector();
    static void            InitAppSelector(const char* uuid);
    static PairState       GetPairState(const char* uuid);
    static void            Pair(const char* uuid);
	static void            MouseMove(int deltaX, int deltaY);
	static void            MouseClick(int buttonId, bool down);
	static void            MouseScroll(signed char amount);
	static void            closeApp(const char* uuid, int appID);

	static long            getLastFrameTimestamp();
	static long            currentTimeMillis();

	static int             addPCbyIP(const char* ip);



};

} // namespace OculusCinema

#endif // Native_h
