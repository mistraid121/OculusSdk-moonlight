/************************************************************************************

Filename    :   PcManager.h
Content     :
Created     :	9/10/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( PcManager_h )
#define PcManager_h

#include "string"
#include "vector"
#include "Native.h"


namespace OculusCinema {

class CinemaApp;

enum PcCategory {
    CATEGORY_LIMELIGHT,
    CATEGORY_REMOTEDESKTOP,
    CATEGORY_VNC
};

class PcDef
{
public:
	std::string            Name;
	std::string            PosterFileName;
	std::string            UUID;
	std::string            Binding;
    int                Id;
	bool            isRunning;
	bool            isRemote;

	GLuint			Poster;
	int				PosterWidth;
	int				PosterHeight;
	PcCategory        Category;


    PcDef() : Name(), PosterFileName(), UUID(), Binding(), Poster( 0 ), PosterWidth( 0 ), PosterHeight( 0 ),
        Category( CATEGORY_LIMELIGHT ) {}


};

class PcManager
{
public:
							PcManager( CinemaApp &cinema );
							~PcManager();

	void					OneTimeInit( const char * launchIntent );
	void					OneTimeShutdown();

    void
    AddPc(const char *name, const char *uuid, Native::PairState pairState, Native::Reachability reachability, const char *binding, const bool isRunning);

	void                    RemovePc(const std::string &name);
	std::vector<const PcDef *>    GetPcList( PcCategory category ) const;

public:
	std::vector<PcDef *> 		Pcs;
    bool                    updated;

private:
	CinemaApp &				Cinema;

	PcManager &			operator=( const PcManager & );

private:
    GLuint                    PcPosterPaired;
    GLuint                    PcPosterUnpaired;
    GLuint                    PcPosterUnknown;
    GLuint                    PcPosterWTF;
    virtual void             ReadMetaData( PcDef *aPc );

};

} // namespace OculusCinema

#endif // MovieManager_h
