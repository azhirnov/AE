// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Public/ImageDesc.h"
#include "graphics/Private/EnumUtils.h"

namespace AE::Graphics
{
	
/*
=================================================
	ImageDesc
=================================================
*/
	ImageDesc::ImageDesc (EImage		imageType,
						  const uint3	&dimension,
						  EPixelFormat	format,
						  EImageUsage	usage,
						  EImageFlags	flags,
						  ImageLayer	arrayLayers,
						  MipmapLevel	maxLevel,
						  MultiSamples	samples,
						  EQueueMask	queues,
						  EMemoryType	memType) :
		imageType{ imageType },		flags{ flags },
		dimension{ dimension },		format{ format },
		usage{ usage },				arrayLayers{ arrayLayers },
		maxLevel{ maxLevel },		samples{ samples },
		queues{ queues },			memType{ memType }
	{}
	
/*
=================================================
	ImageDesc::SetDimensionAndType
=================================================
*/
	ImageDesc&  ImageDesc::SetDimensionAndType (const uint3 &value)
	{
		dimension = value;

		if ( dimension.z > 1 )
			imageType = EImage::_3D;
		else
		if ( dimension.y > 1 )
			imageType = EImage::_2D;
		else
			imageType = EImage::_1D;

		return *this;
	}
	
/*
=================================================
	NumberOfMipmaps
=================================================
*/	
	ND_ static uint  NumberOfMipmaps (const uint3 &dim)
	{
		return IntLog2( Max(Max( dim.x, dim.y ), dim.z )) + 1;
	}

/*
=================================================
	Validate
=================================================
*/
	void ImageDesc::Validate ()
	{
		ASSERT( format != Default );
		ASSERT( imageType != Default );
		
		dimension = Max( dimension, 1u );

		BEGIN_ENUM_CHECKS();
		switch ( imageType )
		{
			case EImage::_1D :
				ASSERT( not samples.IsEnabled() );
				ASSERT( dimension.y == 1 and dimension.z == 1 );
				samples		= 1_samples;
				dimension	= uint3{ dimension.x, 1, 1 };
				break;

			case EImage::_2D :
				ASSERT( dimension.z == 1 );
				dimension.z	= 1;
				break;

			case EImage::_3D :
				samples		= 1_samples;
				arrayLayers	= 1_layer;
				break;

			case EImage::Unknown :
				break;
		}
		END_ENUM_CHECKS();

		if ( samples.IsEnabled() )
		{
			ASSERT( maxLevel <= 1_mipmap );
			maxLevel = 1_mipmap;
		}
		else
		{
			samples  = 1_samples;
			maxLevel = MipmapLevel( Clamp( maxLevel.Get(), 1u, NumberOfMipmaps( dimension ) ));
		}
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	ImageViewDesc
=================================================
*/	
	ImageViewDesc::ImageViewDesc (EImageView		viewType,
								  EPixelFormat		format,
								  MipmapLevel		baseLevel,
								  uint				levelCount,
								  ImageLayer		baseLayer,
								  uint				layerCount,
								  ImageSwizzle		swizzle,
								  EImageAspect		aspectMask) :
		viewType{ viewType },		format{ format },
		baseLevel{ baseLevel },		levelCount{ levelCount },
		baseLayer{ baseLayer },		layerCount{ layerCount },
		swizzle{ swizzle },			aspectMask{ aspectMask }
	{}
	
/*
=================================================
	ImageViewDesc
=================================================
*/
	ImageViewDesc::ImageViewDesc (const ImageDesc &desc) :
		viewType{ desc.imageType },	format{ desc.format },
		baseLevel{},				levelCount{ desc.maxLevel.Get() },
		baseLayer{},				layerCount{ desc.arrayLayers.Get() },
		swizzle{ "RGBA"_swizzle },	aspectMask{ EPixelFormat_ToImageAspect( format )}
	{}
	
/*
=================================================
	ImageViewDesc::Validate
=================================================
*/
	void ImageViewDesc::Validate (const ImageDesc &desc)
	{
		const uint	max_layers	= desc.arrayLayers.Get();

		baseLayer	= ImageLayer{Clamp( baseLayer.Get(), 0u, max_layers-1 )};
		layerCount	= Clamp( layerCount, 1u, max_layers - baseLayer.Get() );

		baseLevel	= MipmapLevel{Clamp( baseLevel.Get(), 0u, desc.maxLevel.Get()-1 )};
		levelCount	= Clamp( levelCount, 1u, desc.maxLevel.Get() - baseLevel.Get() );

		auto	mask = EPixelFormat_ToImageAspect( format );
		aspectMask   = (aspectMask == Default ? mask : aspectMask & mask);

		if ( viewType == Default )
		{
			BEGIN_ENUM_CHECKS();
			switch ( desc.imageType )
			{
				case EImage::_1D :
					if ( layerCount > 1 )
						viewType = EImageView::_1DArray;
					else
						viewType = EImageView::_1D;
					break;

				case EImage::_2D :
					if ( layerCount > 6 and EnumEq( desc.flags, EImageFlags::CubeCompatibple ))
						viewType = EImageView::CubeArray;
					else
					if ( layerCount == 6 and EnumEq( desc.flags, EImageFlags::CubeCompatibple ))
						viewType = EImageView::Cube;
					else
					if ( layerCount > 1 )
						viewType = EImageView::_2DArray;
					else
						viewType = EImageView::_2D;
					break;

				case EImage::_3D :
					viewType = EImageView::_3D;
					break;

				case EImage::Unknown :
					break;
			}
			END_ENUM_CHECKS();
		}

		if ( format == Default )
			format = desc.format;

		ASSERT( aspectMask != Default );
	}

/*
=================================================
	ImageViewDesc::operator ==
=================================================
*/
	bool ImageViewDesc::operator == (const ImageViewDesc &rhs) const
	{
		return	(this->viewType		== rhs.viewType)	&
				(this->format		== rhs.format)		&
				(this->baseLevel	== rhs.baseLevel)	&
				(this->levelCount	== rhs.levelCount)	&
				(this->baseLayer	== rhs.baseLayer)	&
				(this->layerCount	== rhs.layerCount)	&
				(this->aspectMask	== rhs.aspectMask)	&
				(this->swizzle		== rhs.swizzle);
	}

}	// AE::Graphics

namespace std
{
/*
=================================================
	hash<ImageViewDesc>::operator ()
=================================================
*/
	size_t  hash< AE::Graphics::ImageViewDesc >::operator () (const AE::Graphics::ImageViewDesc &value) const
	{
	#if AE_FAST_HASH
		return size_t(AE::STL::HashOf( AddressOf(value), sizeof(value) ));
	#else
		AE::STL::HashVal	result;
		result << AE::STL::HashOf( value.viewType );
		result << AE::STL::HashOf( value.format );
		result << AE::STL::HashOf( value.baseLevel );
		result << AE::STL::HashOf( value.levelCount );
		result << AE::STL::HashOf( value.baseLayer );
		result << AE::STL::HashOf( value.layerCount );
		result << AE::STL::HashOf( value.swizzle );
		result << AE::STL::HashOf( value.aspectMask );
		return size_t(result);
	#endif
	}

}	// std
