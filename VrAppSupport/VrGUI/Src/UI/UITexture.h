/************************************************************************************

Filename    :   UITexture.h
Content     :
Created     :	1/8/2015
Authors     :   Jim Dose

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#if !defined( UITexture_h )
#define UITexture_h

#include "OVR_GlUtils.h"
#include "OVR_FileSys.h"

#include <vector>

namespace OVR {

class UITexture
{
public:
										UITexture();
	virtual								~UITexture();

	void 								Free();

	void 								LoadTextureFromApplicationPackage( const char * assetPath );	// Depricated.  Use LoadTextureFromUri.
	void 								LoadTextureFromUri( ovrFileSys & fileSys, const char * uri );
	void 								LoadTextureFromBuffer( const char * fileName, const std::vector< uint8_t > & buffer );
	void 								LoadTextureFromMemory( const uint8_t * data, const int width, const int height );

	void								SetTexture( const GLuint texture, const int width, const int height, const bool freeTextureOnDestruct );


	int 								Width;
	int									Height;
	GLuint 								Texture;
	bool								FreeTextureOfDestruct;
};

} // namespace OVR

#endif // UITexture_h
