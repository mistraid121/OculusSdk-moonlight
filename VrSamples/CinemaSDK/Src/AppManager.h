/************************************************************************************

Filename    :   AppManager.h
Content     :
Created     :	9/10/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( AppManager_h )
#define AppManager_h

#include <vector>
#include <string>
#include "PcManager.h"
#include <GLES3/gl3.h>

namespace OculusCinema {

class CinemaApp;

class AppDef
{
public:
	std::string            Name;
	std::string            PosterFileName;
	std::string            UUID;
	std::string            Binding;
	int                Id;
	bool            isRunning;
	GLuint			Poster;
	int				PosterWidth;
	int				PosterHeight;

	AppDef() : Name(), PosterFileName(), UUID(), Binding(), Poster( 0 ), PosterWidth( 0 ), PosterHeight( 0 ){}
};

class AppManager
{
public:
							AppManager( CinemaApp &cinema );
	virtual					~AppManager();

	virtual void			OneTimeInit( const char * launchIntent );
	virtual void			OneTimeShutdown();
	void                    LoadPosters();
	void                    AddApp(const std::string &name, const std::string &posterFileName, int id, bool isRunning);
	void					RemoveApp( int id);

	std::vector<const AppDef *>	GetAppList( PcCategory category ) const;

public:
    std::vector<AppDef *> 		Apps;

private:
	CinemaApp &				Cinema;
    GLuint					DefaultPoster;
    virtual void 			ReadMetaData( AppDef *app );
    virtual void 			LoadPoster( AppDef *app );
};

} // namespace OculusCinema

#endif // AppManager_h
