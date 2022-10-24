/************************************************************************************

Filename    :   PackageFiles.cpp
Content     :   Read files from the application package zip
Created     :   August 18, 2014
Authors     :   John Carmack

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.


*************************************************************************************/

#include "PackageFiles.h"

#include "OVR_LogUtils.h"

#include "unzip.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined( OVR_OS_ANDROID )
#include <unistd.h>
#endif

#include <thread>
#include <mutex>
#include <functional>

namespace OVR
{

// Decompressed files can be written here for faster access next launch
static char CachePath[1024];

const char *	ovr_GetApplicationPackageCachePath()
{
	return CachePath;
}

OvrApkFile::OvrApkFile( void * zipFile ) : 
	ZipFile( zipFile ) 
{ 
}

OvrApkFile::~OvrApkFile()
{
	ovr_CloseOtherApplicationPackage( ZipFile );
}

//--------------------------------------------------------------
// Functions for reading assets from other application packages
//--------------------------------------------------------------

void * ovr_OpenOtherApplicationPackage( const char * packageCodePath )
{
	void * zipFile = unzOpen( packageCodePath );

// enable the following block if you need to see the list of files in the application package
// This is useful for finding a file added in one of the res/ sub-folders (necesary if you want
// to include a resource file in every project that links VrAppFramework).
#if 0
	// enumerate the files in the package for us so we can see if the vrappframework res/raw files are in there
	if ( unzGoToFirstFile( zipFile ) == UNZ_OK )
	{
		LOG( "FilesInPackage", "Files in package:" );
		do
		{
			unz_file_info fileInfo;
			char fileName[512];
			if ( unzGetCurrentFileInfo( zipFile, &fileInfo, fileName, sizeof( fileName ), NULL, 0, NULL, 0 ) == UNZ_OK )
			{
				LOG( "FilesInPackage", "%s", fileName );
			}
		} while ( unzGoToNextFile( zipFile ) == UNZ_OK );
	}
#endif
	return zipFile;
}

void ovr_CloseOtherApplicationPackage( void * & zipFile )
{
	if ( zipFile == 0 )
	{
		return;
	}
	unzClose( zipFile );
	zipFile = 0;
}

static std::mutex PackageFileMutex;

bool ovr_OtherPackageFileExists( void* zipFile, const char * nameInZip )
{
	std::lock_guard<std::mutex> mutex( PackageFileMutex );

	const int locateRet = unzLocateFile( zipFile, nameInZip, 2 /* case insensitive */ );
	if ( locateRet != UNZ_OK )
	{
		OVR_LOG( "File '%s' not found in apk!", nameInZip );
		return false;
	}

	const int openRet = unzOpenCurrentFile( zipFile );
	if ( openRet != UNZ_OK )
	{
		OVR_WARN( "Error opening file '%s' from apk!", nameInZip );
		return false;
	}

	unzCloseCurrentFile( zipFile );

	return true;
}

static bool ovr_ReadFileFromOtherApplicationPackageInternal( void * zipFile, const char * nameInZip, int & length, void * & buffer, 
		std::function< void* ( const size_t size ) > allocBuffer, std::function< void ( void * buffer ) > freeBuffer )
{
	length = 0;
	buffer = NULL;
	if ( zipFile == 0 )
	{
		return false;
	}

	std::lock_guard<std::mutex> mutex( PackageFileMutex );

	const int locateRet = unzLocateFile( zipFile, nameInZip, 2 /* case insensitive */ );

	if ( locateRet != UNZ_OK )
	{
		OVR_LOG( "File '%s' not found in apk!", nameInZip );
		return false;
	}

	unz_file_info	info;
	const int getRet = unzGetCurrentFileInfo( zipFile, &info, NULL,0, NULL,0, NULL,0);

	if ( getRet != UNZ_OK )
	{
		OVR_WARN( "File info error reading '%s' from apk!", nameInZip );
		return false;
	}

	// Check for an already extracted cache file based on the CRC if
	// the file is compressed.
	if ( info.compression_method != 0 && CachePath[0] )
	{
		char	cacheName[1024];
		sprintf( cacheName, "%s/%08x.bin", CachePath, (unsigned)info.crc );
#if defined( OVR_OS_ANDROID )
		const int fd = open( cacheName, O_RDONLY );
		if ( fd > 0 )
		{
			struct stat	s = {};

			if ( fstat( fd, &s ) != -1 )
			{
//				LOG( "Loading cached file for: %s", nameInZip );
				length = s.st_size;
				if ( length != (int)info.uncompressed_size )
				{
					OVR_LOG( "Cached file for %s has length %i != %lu", nameInZip,
							length, info.uncompressed_size );
					// Fall through to normal load.
				}
				else
				{
					buffer = allocBuffer( length );
					const int r = read( fd, buffer, length );
					close( fd );
					if ( r != length )
					{
						OVR_LOG( "Cached file for %s only read %i != %i", nameInZip,
								r, length );
						freeBuffer( buffer );
						// Fall through to normal load.
					}
					else
					{	// Got the cached file.
						return true;
					}
				}
			}
			close( fd );
		}
#endif
	}
	else
	{
//		LOG( "Not compressed: %s", nameInZip );
	}

	const int openRet = unzOpenCurrentFile( zipFile );
	if ( openRet != UNZ_OK )
	{
		OVR_WARN( "Error opening file '%s' from apk!", nameInZip );
		return false;
	}

	length = info.uncompressed_size;
	buffer = allocBuffer( length );

	const int readRet = unzReadCurrentFile( zipFile, buffer, length );
	if ( readRet != length )
	{
		OVR_WARN( "Error reading file '%s' from apk!", nameInZip );
		freeBuffer( buffer );
		length = 0;
		buffer = NULL;
		return false;
	}

	unzCloseCurrentFile( zipFile );

	// Optionally write out to the cache directory
	if ( info.compression_method != 0 && CachePath[0] )
	{
		char	tempName[1024];
		sprintf( tempName, "%s/%08x.tmp", CachePath, (unsigned)info.crc );

		char	cacheName[1024];
		sprintf( cacheName, "%s/%08x.bin", CachePath, (unsigned)info.crc );
#if defined( OVR_OS_ANDROID )
		const int fd = open( tempName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );
		if ( fd > 0 )
		{
			const int r = write( fd, buffer, length );
			close( fd );
			if ( r == length )
			{
				if ( rename( tempName, cacheName ) == -1 )
				{
					OVR_LOG( "Failed to rename cache file for %s", nameInZip );
				}
				else
				{
					OVR_LOG( "Cache file generated for %s", nameInZip );
				}
			}
			else
			{
				OVR_LOG( "Only wrote %i of %i for cached %s", r, length, nameInZip );
			}
		}
		else
		{
			OVR_LOG( "Failed to open new cache file for %s: %s", nameInZip, tempName );
		}
#endif
	}

	return true;
}

bool ovr_ReadFileFromOtherApplicationPackage( void * zipFile, const char * nameInZip, std::vector< uint8_t > & outBuffer )
{
	// Dummy parameters
	int length = 0;
	void * buffer = nullptr;

	// allocate using buffer resize
	auto allocBuffer = [&] ( const size_t size )
	{
		outBuffer.resize(size);
		return outBuffer.data();
	};

	auto freeBuffer = [&] ( void * buffer )
	{
		outBuffer.resize(0);
	};

	return ovr_ReadFileFromOtherApplicationPackageInternal( zipFile, nameInZip, length, buffer, allocBuffer, freeBuffer );
}

bool ovr_ReadFileFromOtherApplicationPackage( void * zipFile, const char * nameInZip, int & length, void * & buffer )
{
	// allocate using malloc / free
	auto allocBuffer = [] ( const size_t size )
	{
		return malloc( size );
	};

	auto freeBuffer = [] ( void * buffer )
	{
		free( buffer );
	};

	return ovr_ReadFileFromOtherApplicationPackageInternal( zipFile, nameInZip, length, buffer, allocBuffer, freeBuffer );
}

//--------------------------------------------------------------
// Functions for reading assets from this process's application package
//--------------------------------------------------------------

static unzFile packageZipFile = 0;

void * ovr_GetApplicationPackageFile()
{
	return packageZipFile;
}

void ovr_OpenApplicationPackage( const char * packageCodePath, const char * cachePath_ )
{
	if ( packageZipFile )
	{
		return;
	}
	if ( cachePath_ != NULL )
	{
		OVR_strncpy( CachePath, sizeof( CachePath ), cachePath_, sizeof( CachePath ) - 1 );
	}
	packageZipFile = ovr_OpenOtherApplicationPackage( packageCodePath );
}

bool ovr_PackageFileExists( const char * nameInZip )
{
	return ovr_OtherPackageFileExists( packageZipFile, nameInZip );
}

bool ovr_ReadFileFromApplicationPackage( const char * nameInZip, int & length, void * & buffer )
{
	return ovr_ReadFileFromOtherApplicationPackage( packageZipFile, nameInZip, length, buffer );
}

bool ovr_ReadFileFromApplicationPackage( const char * nameInZip, std::vector< uint8_t > & buffer )
{

	return ovr_ReadFileFromOtherApplicationPackage( packageZipFile, nameInZip, buffer );
}

} // namespace OVR
