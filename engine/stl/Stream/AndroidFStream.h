// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "stl/Stream/Stream.h"

#ifdef PLATFORM_ANDROID

# include <asset_manager.h>
# include <asset_manager_jni.h>

namespace AE::STL
{

	//
	// Read-only Android File Stream
	//

	class AFileRStream final : public RStream
	{
	// variables
	private:
		AAsset *	_asset		= null;
		BytesU		_fileSize;
		BytesU		_position;


	// methods
	public:
		explicit AFileRStream (AAsset* asset) :
			_asset{ asset },
			_fileSize{ asset ? AAsset_getLength(asset) : 0 }
		{}

		~AFileRStream ()
		{
			if ( _asset )
				AAsset_close( _asset );
		}

		bool	IsOpen ()	const override		{ return _asset != null; }
		BytesU	Position ()	const override		{ return _position; }
		BytesU	Size ()		const override		{ return _fileSize; }
		
		bool	SeekSet (BytesU pos) override
		{
			return AAsset_seek( _asset, size_t(pos), SEEK_SET ) != -1;
		}

		BytesU	Read2 (OUT void *buffer, BytesU size) override
		{
			return BytesU{ AAsset_read( _asset, buffer, size_t(size) )};
		}
	};

}	// AE::STL

#endif	// PLATFORM_ANDROID
