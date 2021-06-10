// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Public/DescriptorSet.h"
#include "graphics/Private/DescriptorSetHelper.h"
#include "stl/Memory/UntypedAllocator.h"
#include "stl/Memory/MemWriter.h"

#if 1
#	define UNIFORM_EXISTS	ASSERT
#else
#	define UNIFORM_EXISTS( ... )
#endif

namespace AE::STL
{
namespace {
/*
=================================================
	HashOf (Buffer)
=================================================
*/
	inline HashVal  HashOf (const AE::Graphics::DescriptorSet::Buffer &buf)
	{
		HashVal	result = STL::HashOf( buf.index ) + STL::HashOf( buf.state ) +
						 STL::HashOf( buf.dynamicOffsetIndex ) + STL::HashOf( buf.elementCount );

		for (uint16_t i = 0; i < buf.elementCount; ++i)
		{
			auto&	elem = buf.elements[i];
			result << (STL::HashOf( elem.bufferId ) + STL::HashOf( elem.offset ) + STL::HashOf( elem.size ));
		}
		return result;
	}

/*
=================================================
	HashOf (TexelBuffer)
=================================================
*/
	inline HashVal  HashOf (const AE::Graphics::DescriptorSet::TexelBuffer &texbuf)
	{
		HashVal	result = STL::HashOf( texbuf.index ) + STL::HashOf( texbuf.state ) + STL::HashOf( texbuf.elementCount );

		for (uint16_t i = 0; i < texbuf.elementCount; ++i)
		{
			auto&	elem = texbuf.elements[i];
			result << (STL::HashOf( elem.bufferId ) + STL::HashOf( elem.desc ));
		}
		return result;
	}
	
/*
=================================================
	HashOf (Image)
=================================================
*/
	inline HashVal  HashOf (const AE::Graphics::DescriptorSet::Image &img)
	{
		HashVal	result = STL::HashOf( img.index ) + STL::HashOf( img.state ) + STL::HashOf( img.elementCount );

		for (uint16_t i = 0; i < img.elementCount; ++i)
		{
			auto&	elem = img.elements[i];
			result << (STL::HashOf( elem.imageId ) + (elem.hasDesc ? STL::HashOf( elem.desc ) : HashVal{}));
		}
		return result;
	}
	
/*
=================================================
	HashOf (Texture)
=================================================
*/
	inline HashVal  HashOf (const AE::Graphics::DescriptorSet::Texture &tex)
	{
		HashVal	result = STL::HashOf( tex.index ) + STL::HashOf( tex.state ) + STL::HashOf( tex.elementCount );

		for (uint16_t i = 0; i < tex.elementCount; ++i)
		{
			auto&	elem = tex.elements[i];
			result << (STL::HashOf( elem.imageId ) + STL::HashOf( elem.sampler ) +
					   (elem.hasDesc ? STL::HashOf( elem.desc ) : HashVal{}));
		}
		return result;
	}
	
/*
=================================================
	HashOf (Sampler)
=================================================
*/
	inline HashVal  HashOf (const AE::Graphics::DescriptorSet::Sampler &samp)
	{
		HashVal	result = STL::HashOf( samp.index ) + STL::HashOf( samp.elementCount );

		for (uint16_t i = 0; i < samp.elementCount; ++i)
		{
			result << STL::HashOf( samp.elements[i].sampler );
		}
		return result;
	}

/*
=================================================
	HashOf (RayTracingScene)
=================================================
*
	inline HashVal  HashOf (const AE::Graphics::DescriptorSet::RayTracingScene &rts)
	{
		HashVal	result = STL::HashOf( rts.index ) + STL::HashOf( rts.elementCount );

		for (uint16_t i = 0; i < rts.elementCount; ++i)
		{
			result << STL::HashOf( rts.elements[i].sceneId );
		}
		return result;
	}*/

}	// namespace
}	// AE::STL

namespace AE::Graphics
{

/*
=================================================
	constructor
=================================================
*/
	DescriptorSet::DescriptorSet (DescriptorSet &&other) :
		_index{ other._index },
		_allowEmptyResources{ other._allowEmptyResources },
		_changed{ true }
	{
		_dataPtr = RVRef(other._dataPtr);
	}

/*
=================================================
	destructor
=================================================
*/
	DescriptorSet::~DescriptorSet ()
	{
	}
	
/*
=================================================
	Clone
=================================================
*/
	DescriptorSet  DescriptorSet::Clone () const
	{
		DescriptorSet	ds;
		ds._dataPtr				= DescriptorSetHelper::CloneDynamicData( *this );
		ds._index				= _index;
		ds._allowEmptyResources	= _allowEmptyResources;
		ds._changed				= true;
		return ds;
	}

/*
=================================================
	_GetResourceType
=================================================
*/
	template <typename T>
	ND_ inline bool  DescriptorSet::_CheckResourceType (EDescriptorType type)
	{
		if constexpr( IsSameTypes< T, Image >)
			return	(type == EDescriptorType::StorageImage) | (type == EDescriptorType::SampledImage) |
					(type == EDescriptorType::CombinedImage_ImmutableSampler) | (type == EDescriptorType::SubpassInput);
		else
		if constexpr( IsSameTypes< T, Texture >)
			return	(type == EDescriptorType::CombinedImage);
		else
		if constexpr( IsSameTypes< T, Buffer >)
			return	(type == EDescriptorType::UniformBuffer) | (type == EDescriptorType::StorageBuffer);
		else
		if constexpr( IsSameTypes< T, TexelBuffer >)
			return	(type == EDescriptorType::UniformTexelBuffer) | (type == EDescriptorType::StorageTexelBuffer);
		else
		if constexpr( IsSameTypes< T, Sampler >)
			return	(type == EDescriptorType::Sampler);
	}

/*
=================================================
	_GetResource
=================================================
*/
	template <typename T>
	ND_ inline T*  DescriptorSet::_GetResource (const UniformName &name)
	{
		if ( _dataPtr )
		{
			auto*	uniforms = _dataPtr->Uniforms();
			usize	index	 = BinarySearch( ArrayView<Uniform>{uniforms, _dataPtr->uniformCount}, name );
		
			if ( index < _dataPtr->uniformCount )
			{
				auto&	un = uniforms[ index ];
				ASSERT( _CheckResourceType<T>( un.resType ));

				return Cast<T>( _dataPtr.get() + Bytes{un.offset} );
			}
		}
		return null;
	}
	
/*
=================================================
	_HasResource
=================================================
*/
	template <typename T>
	ND_ bool  DescriptorSet::_HasResource (const UniformName &name) const
	{
		if ( not _dataPtr )
			return false;
		
		auto*	uniforms = _dataPtr->Uniforms();
		usize	index	 = BinarySearch( ArrayView<Uniform>{uniforms, _dataPtr->uniformCount}, name );

		if ( index >= _dataPtr->uniformCount )
			return false;

		return _CheckResourceType<T>( uniforms[ index ].resType );
	}

	bool  DescriptorSet::HasImage (const UniformName &name)				const	{ return _HasResource< Image >( name ); }
	bool  DescriptorSet::HasSampler (const UniformName &name)			const	{ return _HasResource< Sampler >( name ); }
	bool  DescriptorSet::HasTexture (const UniformName &name)			const	{ return _HasResource< Texture >( name ); }
	bool  DescriptorSet::HasBuffer (const UniformName &name)			const	{ return _HasResource< Buffer >( name ); }
	bool  DescriptorSet::HasTexelBuffer (const UniformName &name)		const	{ return _HasResource< TexelBuffer >( name ); }
	//bool  DescriptorSet::HasRayTracingScene (const UniformName &name)	const	{ return _HasResource< RayTracingScene >( name ); }
	
/*
=================================================
	GetType
=================================================
*/
	DescriptorSet::EDescriptorType  DescriptorSet::GetType (const UniformName &name) const
	{
		if ( _dataPtr )
		{
			auto*	uniforms = _dataPtr->Uniforms();
			usize	index	 = BinarySearch( ArrayView<Uniform>{uniforms, _dataPtr->uniformCount}, name );

			return uniforms[index].resType;
		}
		return EDescriptorType::Unknown;
	}

/*
=================================================
	BindImage
=================================================
*/
	DescriptorSet&  DescriptorSet::BindImage (const UniformName &name, ImageID image, uint index)
	{
		UNIFORM_EXISTS( HasImage( name ));

		if ( auto* res = _GetResource<Image>( name ))
		{
			auto&	img = res->elements[ index ];
			ASSERT( index < res->elementCapacity );

			if ( img.imageId != image or img.hasDesc or res->elementCount <= index )
				_changed = true;
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			img.imageId			= image;
			img.hasDesc			= false;
		}
		return *this;
	}
	
	DescriptorSet&  DescriptorSet::BindImage (const UniformName &name, ImageID image, const ImageViewDesc &desc, uint index)
	{
		UNIFORM_EXISTS( HasImage( name ));

		if ( auto* res = _GetResource<Image>( name ))
		{
			auto&	img = res->elements[ index ];
			ASSERT( index < res->elementCapacity );

			if ( img.imageId != image or not img.hasDesc or not (img.desc == desc) or res->elementCount <= index )
				_changed = true;
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			img.imageId			= image;
			img.desc			= desc;
			img.hasDesc			= true;
		}
		return *this;
	}
	
/*
=================================================
	BindImages
=================================================
*/
	DescriptorSet&  DescriptorSet::BindImages (const UniformName &name, ArrayView<ImageID> images)
	{
		UNIFORM_EXISTS( HasImage( name ));
		ASSERT( images.size() > 0 );

		if ( auto* res = _GetResource<Image>( name ))
		{
			_changed |= (res->elementCount != images.size());
		
			ASSERT( images.size() <= res->elementCapacity );
			res->elementCount = uint16_t(images.size());

			for (usize i = 0; i < images.size(); ++i)
			{
				auto&	img = res->elements[i];

				_changed |= (img.imageId != images[i] or img.hasDesc);

				img.imageId	= images[i];
				img.hasDesc	= false;
			}
		}
		return *this;
	}

/*
=================================================
	BindTexture
=================================================
*/
	DescriptorSet&  DescriptorSet::BindTexture (const UniformName &name, ImageID image, const SamplerName &sampler, uint index)
	{
		UNIFORM_EXISTS( HasTexture( name ));

		if ( auto* res = _GetResource<Texture>( name ))
		{
			auto&	tex = res->elements[ index ];
			ASSERT( index < res->elementCapacity );

			if ( tex.imageId != image or tex.sampler != sampler or tex.hasDesc or res->elementCount <= index )
				_changed = true;
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			tex.imageId			= image;
			tex.sampler			= sampler;
			tex.hasDesc			= false;
		}
		return *this;
	}
	
	DescriptorSet&  DescriptorSet::BindTexture (const UniformName &name, ImageID image, const SamplerName &sampler, const ImageViewDesc &desc, uint index)
	{
		UNIFORM_EXISTS( HasTexture( name ));

		if ( auto* res = _GetResource<Texture>( name ))
		{
			auto&	tex = res->elements[ index ];
			ASSERT( index < res->elementCapacity );

			if ( tex.imageId != image or tex.sampler != sampler or not tex.hasDesc or not (tex.desc == desc) or res->elementCount <= index )
				_changed = true;
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			tex.imageId			= image;
			tex.sampler			= sampler;
			tex.desc			= desc;
			tex.hasDesc			= true;
		}
		return *this;
	}
	
/*
=================================================
	BindTextures
=================================================
*/
	DescriptorSet&  DescriptorSet::BindTextures (const UniformName &name, ArrayView<ImageID> images, const SamplerName &sampler)
	{
		UNIFORM_EXISTS( HasTexture( name ));
		ASSERT( images.size() > 0 );

		if ( auto* res = _GetResource<Texture>( name ))
		{
			_changed |= (res->elementCount != images.size());

			ASSERT( images.size() <= res->elementCapacity );
			res->elementCount = uint16_t(images.size());

			for (usize i = 0; i < images.size(); ++i)
			{
				auto&	tex = res->elements[i];

				_changed |= (tex.imageId != images[i] or tex.sampler != sampler or tex.hasDesc);

				tex.imageId	= images[i];
				tex.sampler	= sampler;
				tex.hasDesc	= false;
			}
		}
		return *this;
	}

/*
=================================================
	BindSampler
=================================================
*/
	DescriptorSet&  DescriptorSet::BindSampler (const UniformName &name, const SamplerName &sampler, uint index)
	{
		UNIFORM_EXISTS( HasSampler( name ));
		ASSERT( sampler.IsDefined() );

		if ( auto* res = _GetResource<Sampler>( name ))
		{
			auto&	samp = res->elements[ index ];
			ASSERT( index < res->elementCapacity );

			if ( samp.sampler != sampler or res->elementCount <= index )
				_changed = true;
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			samp.sampler		= sampler;
		}
		return *this;
	}
	
/*
=================================================
	BindSamplers
=================================================
*/
	DescriptorSet&  DescriptorSet::BindSamplers (const UniformName &name, ArrayView<SamplerName> samplers)
	{
		UNIFORM_EXISTS( HasSampler( name ));
		ASSERT( samplers.size() > 0 );

		if ( auto* res = _GetResource<Sampler>( name ))
		{
			_changed |= res->elementCount != samplers.size();
		
			ASSERT( samplers.size() <= res->elementCapacity );
			res->elementCount = uint16_t(samplers.size());

			for (usize i = 0; i < samplers.size(); ++i)
			{
				ASSERT( samplers[i].IsDefined() );

				auto&	samp = res->elements[i];

				_changed |= (samp.sampler != samplers[i]);

				samp.sampler = samplers[i];
			}
		}
		return *this;
	}

/*
=================================================
	BindBuffer
=================================================
*/
	DescriptorSet&  DescriptorSet::BindBuffer (const UniformName &name, BufferID buffer, uint index)
	{
		return BindBuffer( name, buffer, 0_b, ~0_b, index );
	}

	DescriptorSet&  DescriptorSet::BindBuffer (const UniformName &name, BufferID buffer, Bytes offset, Bytes size, uint index)
	{
		UNIFORM_EXISTS( HasBuffer( name ));

		if ( auto* res = _GetResource<Buffer>( name ))
		{
			auto&	buf	= res->elements[ index ];

			if ( size != ~0_b )
			{
				if ( res->arrayStride == 0 ) {
					ASSERT( size == res->staticSize );
				} else {
					ASSERT( size >= res->staticSize );
					ASSERT( (size - res->staticSize) % res->arrayStride == 0 );
				}
			}

			ASSERT( index < res->elementCapacity );

			_changed |= (buf.bufferId != buffer or buf.size != size or res->elementCount <= index);
		
			if ( res->dynamicOffsetIndex == UMax )
			{
				_changed	|= (buf.offset != offset);
				buf.offset	 = offset;
			}
			else
			{
				ASSERT( offset >= buf.offset and offset - buf.offset < std::numeric_limits<uint>::max() );
				_GetDynamicOffset( res->dynamicOffsetIndex + index ) = uint(offset - buf.offset);
			}
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			buf.bufferId		= buffer;
			buf.size			= size;
		}
		return *this;
	}
	
/*
=================================================
	BindBuffers
=================================================
*/
	DescriptorSet&  DescriptorSet::BindBuffers (const UniformName &name, ArrayView<BufferID> buffers)
	{
		UNIFORM_EXISTS( HasBuffer( name ));
		ASSERT( buffers.size() > 0 );
		
		if ( auto* res = _GetResource<Buffer>( name ))
		{
			_changed |= (res->elementCount != buffers.size());

			const Bytes		offset	= 0_b;
			const Bytes		size	= ~0_b;	// whole size
		
			ASSERT( buffers.size() <= res->elementCapacity );
			res->elementCount = uint16_t(buffers.size());

			for (usize i = 0; i < buffers.size(); ++i)
			{
				auto&	buf = res->elements[i];

				_changed |= (buf.bufferId != buffers[i] or buf.size != size);
		
				if ( res->dynamicOffsetIndex == UMax )
				{
					_changed	|= (buf.offset != offset);
					buf.offset	 = offset;
				}
				else
				{
					ASSERT( (offset >= buf.offset) and (offset - buf.offset < std::numeric_limits<uint>::max()) );
					_GetDynamicOffset( res->dynamicOffsetIndex + uint(i) ) = uint(offset - buf.offset);
				}

				buf.bufferId = buffers[i];
				buf.size	 = size;
			}
		}
		return *this;
	}

/*
=================================================
	SetBufferBase
=================================================
*/
	DescriptorSet&  DescriptorSet::SetBufferBase (const UniformName &name, Bytes offset, uint index)
	{
		UNIFORM_EXISTS( HasBuffer( name ));

		if ( auto* res = _GetResource<Buffer>( name ))
		{
			auto&	buf	= res->elements[ index ];

			_changed |= (res->elementCount <= index);

			ASSERT( index < res->elementCapacity );
			res->elementCount = Max( uint16_t(index+1), res->elementCount );

			if ( res->dynamicOffsetIndex != UMax )
			{
				_changed |= (buf.offset != offset);
				_GetDynamicOffset( res->dynamicOffsetIndex + index ) = uint(_GetDynamicOffset( res->dynamicOffsetIndex + index ) + buf.offset - offset);
				buf.offset = offset;
			}
		}
		return *this;
	}
	
/*
=================================================
	BindTexelBuffer
=================================================
*/
	DescriptorSet&  DescriptorSet::BindTexelBuffer (const UniformName &name, BufferID buffer, const BufferViewDesc &desc, uint index)
	{
		UNIFORM_EXISTS( HasTexelBuffer( name ));

		if ( auto* res = _GetResource<TexelBuffer>( name ))
		{
			auto&	texbuf = res->elements[ index ];

			_changed |= (res->elementCount <= index);
			_changed |= ((texbuf.bufferId != buffer) or not (texbuf.desc == desc));

			ASSERT( index < res->elementCapacity );
			res->elementCount = Max( uint16_t(index+1), res->elementCount );

			texbuf.bufferId	= buffer;
			texbuf.desc		= desc;
		}
		return *this;
	}

/*
=================================================
	BindRayTracingScene
=================================================
*
	DescriptorSet&  DescriptorSet::BindRayTracingScene (const UniformName &name, RawRTSceneID scene, uint index)
	{
		UNIFORM_EXISTS( HasRayTracingScene( name ));
		ASSERT( scene.ResourceType() == GfxResourceID::EType::RayTracingScene );

		if ( auto* res = _GetResource<RayTracingScene>( name ))
		{
			auto&	rts	= res->elements[ index ];
			ASSERT( index < res->elementCapacity );

			if ( rts.sceneId != scene or res->elementCount <= index )
				_changed = true;
		
			res->elementCount	= Max( uint16_t(index+1), res->elementCount );
			rts.sceneId			= scene;
		}
		return *this;
	}*/
	
/*
=================================================
	Reset
=================================================
*/
	void  DescriptorSet::Reset (const UniformName &name)
	{
		CHECK_ERRV( _dataPtr );
		
		auto*	uniforms = _dataPtr->Uniforms();
		usize	index	 = BinarySearch( ArrayView<Uniform>{uniforms, _dataPtr->uniformCount}, name );
		
		CHECK_ERRV( index < _dataPtr->uniformCount )
		
		auto&	un  = uniforms[ index ];
		void*	ptr = _dataPtr.get() + Bytes{un.offset};

		BEGIN_ENUM_CHECKS();
		switch ( un.resType )
		{
			case EDescriptorType::Unknown :							break;
			case EDescriptorType::UniformBuffer :
			case EDescriptorType::StorageBuffer :					Cast<Buffer>(ptr)->elementCount = 0;			break;
			case EDescriptorType::UniformTexelBuffer :
			case EDescriptorType::StorageTexelBuffer :				Cast<TexelBuffer>(ptr)->elementCount = 0;		break;
			case EDescriptorType::SubpassInput :
			case EDescriptorType::StorageImage :
			case EDescriptorType::SampledImage :
			case EDescriptorType::CombinedImage_ImmutableSampler :	Cast<Image>(ptr)->elementCount = 0;				break;
			case EDescriptorType::CombinedImage :					Cast<Texture>(ptr)->elementCount = 0;			break;
			case EDescriptorType::Sampler :							Cast<Sampler>(ptr)->elementCount = 0;			break;
			case EDescriptorType::RayTracingScene :					//Cast<RayTracingScene>(ptr)->elementCount = 0;	break;
			case EDescriptorType::ImmutableSampler :
			default :												CHECK(false);	break;
		}
		END_ENUM_CHECKS();
		
		_changed = true;
	}
	
/*
=================================================
	ResetAll
=================================================
*/
	void  DescriptorSet::ResetAll ()
	{
		CHECK_ERRV( _dataPtr );

		_dataPtr->ForEachUniform( [](auto&, auto, auto& data) { data.elementCount = 0; });
		_changed = true;
	}
//-----------------------------------------------------------------------------


namespace {
/*
=================================================
	operator == (Buffer)
=================================================
*/
	inline bool  operator == (const DescriptorSet::Buffer &lhs, const DescriptorSet::Buffer &rhs)
	{
		if ( lhs.index				!= rhs.index				or
			 lhs.state				!= rhs.state				or
			 lhs.dynamicOffsetIndex	!= rhs.dynamicOffsetIndex	or
			// lhs.staticSize		!= rhs.staticSize			or
			// lhs.arrayStride		!= rhs.arrayStride			or
			 lhs.elementCount		!= rhs.elementCount )
			return false;

		for (uint16_t i = 0; i < lhs.elementCount; ++i)
		{
			if ( lhs.elements[i].bufferId	!= rhs.elements[i].bufferId or
				 lhs.elements[i].offset		!= rhs.elements[i].offset	or
				 lhs.elements[i].size		!= rhs.elements[i].size )
				return false;
		}
		return true;
	}
/*
=================================================
	operator == (TexelBuffer)
=================================================
*/
	inline bool  operator == (const DescriptorSet::TexelBuffer &lhs, const DescriptorSet::TexelBuffer &rhs)
	{
		if ( lhs.index			!= rhs.index		or
			 lhs.state			!= rhs.state		or
			 lhs.elementCount	!= rhs.elementCount )
			return false;

		for (uint16_t i = 0; i < lhs.elementCount; ++i)
		{
			if ( lhs.elements[i].bufferId	!= rhs.elements[i].bufferId or
				 not (lhs.elements[i].desc	== rhs.elements[i].desc) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	operator == (Image)
=================================================
*/
	inline bool  operator == (const DescriptorSet::Image &lhs, const DescriptorSet::Image &rhs)
	{
		if ( lhs.index			!= rhs.index		or
			 lhs.state			!= rhs.state		or
			 lhs.elementCount	!= rhs.elementCount )
			return false;

		for (uint16_t i = 0; i < lhs.elementCount; ++i)
		{
			if ( lhs.elements[i].imageId	!= rhs.elements[i].imageId	or
				 lhs.elements[i].hasDesc	!= rhs.elements[i].hasDesc	or
				 (lhs.elements[i].hasDesc	?  not (lhs.elements[i].desc == rhs.elements[i].desc) : false) )
				return false;
		}
		return true;
	}
	
/*
=================================================
	operator == (Texture)
=================================================
*/
	inline bool  operator == (const DescriptorSet::Texture &lhs, const DescriptorSet::Texture &rhs)
	{
		if ( lhs.index			!= rhs.index		or
			 lhs.state			!= rhs.state		or
			 lhs.elementCount	!= rhs.elementCount	)
			return false;

		for (uint16_t i = 0; i < lhs.elementCount; ++i)
		{
			if ( lhs.elements[i].imageId	!= rhs.elements[i].imageId		or
				 lhs.elements[i].sampler	!= rhs.elements[i].sampler	or
				 lhs.elements[i].hasDesc	!= rhs.elements[i].hasDesc		or
				 (lhs.elements[i].hasDesc	?  not (lhs.elements[i].desc == rhs.elements[i].desc) : false) )
				return false;
		}
		return true;
	}

/*
=================================================
	operator == (Sampler)
=================================================
*/
	inline bool  operator == (const DescriptorSet::Sampler &lhs, const DescriptorSet::Sampler &rhs)
	{
		if ( lhs.index			!= rhs.index		or
			 lhs.elementCount	!= rhs.elementCount )
			return false;

		for (uint16_t i = 0; i < lhs.elementCount; ++i)
		{
			if ( lhs.elements[i].sampler != rhs.elements[i].sampler )
				return false;
		}
		return true;
	}

/*
=================================================
	operator == (RayTracingScene)
=================================================
*
	inline bool  operator == (const DescriptorSet::RayTracingScene &lhs, const DescriptorSet::RayTracingScene &rhs)
	{
		if ( lhs.index			!= rhs.index		or
			 lhs.elementCount	!= rhs.elementCount )
			return false;

		for (uint16_t i = 0; i < lhs.elementCount; ++i)
		{
			if ( lhs.elements[i].sceneId != rhs.elements[i].sceneId )
				return false;
		}
		return true;
	}*/

}	// namespace
//-----------------------------------------------------------------------------



	using Allocator = UntypedAllocator;
/*
=================================================
	DynamicDataDeleter::operator ()
=================================================
*/
	void DescriptorSet::DynamicDataDeleter::operator () (DynamicData *ptr) const
	{
		ASSERT( ptr != null );
		Allocator::Deallocate( ptr, ptr->memSize );
	}

/*
=================================================
	CalcHash
=================================================
*/
	HashVal  DescriptorSet::DynamicData::CalcHash () const
	{
		HashVal	result;
		ForEachUniform( [&result] (const UniformName &name, auto, auto& res) { result << HashOf(name) << HashOf(res); });
		return result;
	}

/*
=================================================
	operator ==
=================================================
*/
	bool  DescriptorSet::DynamicData::operator == (const DynamicData &rhs) const
	{
		auto&	lhs = *this;

		if ( lhs.layoutId		!= rhs.layoutId		and
			 lhs.uniformCount	!= rhs.uniformCount	)
			return false;

		for (uint i = 0, cnt = lhs.uniformCount; i < cnt; ++i)
		{
			auto&		lhs_un  = lhs.Uniforms()[i];
			auto&		rhs_un	= rhs.Uniforms()[i];
			void const*	lhs_ptr	= (&lhs + Bytes{lhs_un.offset});
			void const*	rhs_ptr = (&rhs + Bytes{rhs_un.offset});
			bool		equals	= true;

			if ( lhs_un.name	!= rhs_un.name	or
				 lhs_un.resType != rhs_un.resType )
				return false;

			BEGIN_ENUM_CHECKS();
			switch ( lhs_un.resType )
			{
				case EDescriptorType::Unknown :							break;
				case EDescriptorType::UniformBuffer :
				case EDescriptorType::StorageBuffer :					equals = (*Cast<Buffer>(lhs_ptr)		== *Cast<Buffer>(rhs_ptr));			break;
				case EDescriptorType::UniformTexelBuffer :
				case EDescriptorType::StorageTexelBuffer :				equals = (*Cast<TexelBuffer>(lhs_ptr)	== *Cast<TexelBuffer>(rhs_ptr));		break;
				case EDescriptorType::SubpassInput :
				case EDescriptorType::StorageImage :
				case EDescriptorType::SampledImage :
				case EDescriptorType::CombinedImage_ImmutableSampler :	equals = (*Cast<Image>(lhs_ptr)			  == *Cast<Image>(rhs_ptr));			break;
				case EDescriptorType::CombinedImage :					equals = (*Cast<Texture>(lhs_ptr)		  == *Cast<Texture>(rhs_ptr));			break;
				case EDescriptorType::Sampler :							equals = (*Cast<Sampler>(lhs_ptr)		  == *Cast<Sampler>(rhs_ptr));			break;
				case EDescriptorType::RayTracingScene :					//equals = (*Cast<RayTracingScene>(lhs_ptr) == *Cast<RayTracingScene>(rhs_ptr));	break;
				case EDescriptorType::ImmutableSampler :
				default :												equals = false;	break;
			}
			END_ENUM_CHECKS();

			if ( not equals )
				return false;
		}
		return true;
	}
//-----------------------------------------------------------------------------



/*
=================================================
	Initialize
=================================================
*/
	bool  DescriptorSetHelper::Initialize (OUT DescriptorSet &ds, DescriptorSetLayoutID layoutId, uint index, const DynamicDataPtr &dataPtr)
	{
		CHECK_ERR( dataPtr );

		ds._dataPtr = DynamicDataPtr{ Cast<DescriptorSet::DynamicData>( Allocator::Allocate( dataPtr->memSize ))};
		ds._index	= CheckCast<uint16_t>(index);
		ds._changed = true;

		MemCopy( OUT ds._dataPtr.get(), dataPtr.get(), dataPtr->memSize );
		ds._dataPtr->layoutId = layoutId;

		return true;
	}
	
/*
=================================================
	CloneDynamicData
=================================================
*/
	DescriptorSet::DynamicDataPtr  DescriptorSetHelper::CloneDynamicData (const DescriptorSet &ds)
	{
		if ( not ds._dataPtr )
			return {};

		auto&	data	= ds._dataPtr;
		auto*	result	= Cast<DescriptorSet::DynamicData>( Allocator::Allocate( data->memSize ));
		
		MemCopy( OUT result, data.get(), data->memSize );
		return DynamicDataPtr{ result };
	}
	
/*
=================================================
	RemoveDynamicData
=================================================
*/
	DescriptorSet::DynamicDataPtr  DescriptorSetHelper::RemoveDynamicData (INOUT DescriptorSet &ds)
	{
		auto	temp = RVRef(ds._dataPtr);
		return temp;
	}

/*
=================================================
	CreateDynamicData
=================================================
*/
	DescriptorSet::DynamicDataPtr  DescriptorSetHelper::CreateDynamicData (const Uniforms_t &uniforms, uint bufferDynamicOffsetCount)
	{
		using DS	= DescriptorSet;
		using DST	= DS::EDescriptorType;

		Bytes	req_size  = SizeOf<DS::DynamicData> +
							SizeOf<DS::Uniform> * uniforms.Get<0>() +
							SizeOf<uint> * bufferDynamicOffsetCount;
		
		// calculate required size
		for (usize i = 0; i < uniforms.Get<0>(); ++i)
		{
			const auto&		un				= uniforms.Get<2>()[i];
			const uint16_t	array_capacity	= uint16_t(un.arraySize); // TODO: dynamic indexing

			BEGIN_ENUM_CHECKS();
			switch ( un.type )
			{
				case DST::UniformBuffer :
				case DST::StorageBuffer :
					req_size  = AlignToLarger( req_size, AlignOf<DS::Buffer> );
					req_size += SizeOf<DS::Buffer> + SizeOf<DS::Buffer::Element> * (array_capacity-1);
					break;

				case DST::UniformTexelBuffer :
				case DST::StorageTexelBuffer :
					req_size  = AlignToLarger( req_size, AlignOf<DS::TexelBuffer> );
					req_size += SizeOf<DS::TexelBuffer> + SizeOf<DS::TexelBuffer::Element> * (array_capacity-1);
					break;

				case DST::StorageImage :
				case DST::SampledImage :
				case DST::CombinedImage_ImmutableSampler :
				case DST::SubpassInput :
					req_size  = AlignToLarger( req_size, AlignOf<DS::Image> );
					req_size += SizeOf<DS::Image> + SizeOf<DS::Image::Element> * (array_capacity-1);
					break;

				case DST::CombinedImage :
					req_size  = AlignToLarger( req_size, AlignOf<DS::Texture> );
					req_size += SizeOf<DS::Texture> + SizeOf<DS::Texture::Element> * (array_capacity-1);
					break;

				case DST::Sampler :
					req_size  = AlignToLarger( req_size, AlignOf<DS::Sampler> );
					req_size += SizeOf<DS::Sampler> + SizeOf<DS::Sampler::Element> * (array_capacity-1);
					break;

				case DST::ImmutableSampler :
					break;
				case DST::RayTracingScene :
				case DST::Unknown :
				default :
					CHECK(false);
					break;
			}
			END_ENUM_CHECKS();
		}
	
		MemWriter	mem			{ Allocator::Allocate( req_size ), req_size };
		uint		dbo_count	= 0;
		uint		un_index	= 0;

		auto*	data			= &mem.Emplace<DS::DynamicData>();
		auto*	uniforms_ptr	= mem.EmplaceArray<DS::Uniform>( uniforms.Get<0>() );
		data->memSize			= req_size;
		data->uniformCount		= uint(uniforms.Get<0>());
		data->uniformsOffset	= uint(mem.OffsetOf( uniforms_ptr ));

		auto*	dyn_offsets			= mem.EmplaceArray<uint>( bufferDynamicOffsetCount );
		data->dynamicOffsetsCount	= bufferDynamicOffsetCount;
		data->dynamicOffsetsOffset	= uint(usize(mem.OffsetOf( dyn_offsets )));

		// initialize descriptors
		for (usize i = 0; i < uniforms.Get<0>(); ++i)
		{
			const auto&		un				= uniforms.Get<2>()[i];
			auto&			curr			= uniforms_ptr[ un_index++ ];
			const uint16_t	array_capacity	= uint16_t(un.arraySize); // TODO: dynamic indexing
			const uint16_t	array_size		= uint16_t(un.arraySize);
			
			curr.name		= uniforms.Get<1>()[i];
			curr.resType	= un.type;

			BEGIN_ENUM_CHECKS();
			switch ( un.type )
			{
				case DST::UniformBuffer :
				{
					auto& ubuf = un.buffer;
					auto* ptr  = Cast<DS::Buffer>( mem.Reserve( Bytes{sizeof(DS::Buffer) + sizeof(DS::Buffer::Element) * (array_capacity-1)}, AlignOf<DS::Buffer> ));

					ptr->index				= un.index;
					ptr->state				= ubuf.state;
					ptr->dynamicOffsetIndex	= ubuf.dynamicOffsetIndex;
					ptr->staticSize			= ubuf.staticSize;
					ptr->arrayStride		= 0_b;
					ptr->elementCapacity	= array_capacity;
					ptr->elementCount		= array_size;

					for (uint j = 0; j < array_capacity; ++j) {
						PlacementNew<DS::Buffer::Element>( &ptr->elements[j] );
					}

					dbo_count	+= uint(ubuf.dynamicOffsetIndex != UMax) * array_capacity;
					curr.offset	 = uint16_t(mem.OffsetOf( ptr ));
					break;
				}
				
				case DST::StorageBuffer :
				{
					auto& sbuf = un.buffer;
					auto* ptr  = Cast<DS::Buffer>( mem.Reserve( Bytes{sizeof(DS::Buffer) + sizeof(DS::Buffer::Element) * (array_capacity-1)}, AlignOf<DS::Buffer> ));

					ptr->index				= un.index;
					ptr->state				= sbuf.state;
					ptr->dynamicOffsetIndex	= sbuf.dynamicOffsetIndex;
					ptr->staticSize			= sbuf.staticSize;
					ptr->arrayStride		= sbuf.arrayStride;
					ptr->elementCapacity	= array_capacity;
					ptr->elementCount		= array_size;

					for (uint j = 0; j < array_capacity; ++j) {
						PlacementNew<DS::Buffer::Element>( &ptr->elements[j] );
					}

					dbo_count	+= uint(sbuf.dynamicOffsetIndex != UMax) * array_capacity;
					curr.offset	 = uint16_t(mem.OffsetOf( ptr ));
					break;
				}

				case DST::UniformTexelBuffer :
				case DST::StorageTexelBuffer :
				{
					auto& tbuf = un.texelBuffer;
					auto* ptr  = Cast<DS::TexelBuffer>( mem.Reserve( Bytes{sizeof(DS::TexelBuffer) + sizeof(DS::TexelBuffer::Element) * (array_capacity-1)}, AlignOf<DS::TexelBuffer> ));
					
					ptr->index				= un.index;
					ptr->state				= tbuf.state;
					ptr->elementCapacity	= array_capacity;
					ptr->elementCount		= array_size;
					
					for (uint j = 0; j < array_capacity; ++j) {
						PlacementNew<DS::TexelBuffer::Element>( &ptr->elements[j] );
					}

					curr.offset	= uint16_t(mem.OffsetOf( ptr ));
					break;
				}

				case DST::StorageImage :
				case DST::SampledImage :
				case DST::CombinedImage_ImmutableSampler :
				case DST::SubpassInput :
				{
					auto& img = un.storageImage;
					auto* ptr = Cast<DS::Image>( mem.Reserve( Bytes{sizeof(DS::Image) + sizeof(DS::Image::Element) * (array_capacity-1)}, AlignOf<DS::Image> ));
					
					ptr->index				= un.index;
					ptr->state				= img.state;
					ptr->elementCapacity	= array_capacity;
					ptr->elementCount		= array_size;
					
					for (uint j = 0; j < array_capacity; ++j) {
						PlacementNew<DS::Image::Element>( &ptr->elements[j] );
					}

					curr.offset	= uint16_t(mem.OffsetOf( ptr ));
					break;
				}

				case DST::CombinedImage :
				{
					auto& img = un.combinedImage;
					auto* ptr = Cast<DS::Texture>( mem.Reserve( Bytes{sizeof(DS::Texture) + sizeof(DS::Texture::Element) * (array_capacity-1)}, AlignOf<DS::Texture> ));
					
					ptr->index				= un.index;
					ptr->state				= img.state;
					ptr->elementCapacity	= array_capacity;
					ptr->elementCount		= array_size;
					
					for (uint j = 0; j < array_capacity; ++j) {
						PlacementNew<DS::Texture::Element>( &ptr->elements[j] );
					}

					curr.offset	= uint16_t(mem.OffsetOf( ptr ));
					break;
				}

				case DST::Sampler :
				{
					//auto& samp = un.sampler;
					auto* ptr = Cast<DS::Sampler>( mem.Reserve( Bytes{sizeof(DS::Sampler) + sizeof(DS::Sampler::Element) * (array_capacity-1)}, AlignOf<DS::Sampler> ));
					
					ptr->index				= un.index;
					ptr->elementCapacity	= array_capacity;
					ptr->elementCount		= array_size;
					
					for (uint j = 0; j < array_capacity; ++j) {
						PlacementNew<DS::Sampler::Element>( &ptr->elements[j] );
					}

					curr.offset	= uint16_t(mem.OffsetOf( ptr ));
					break;
				}

				case DST::ImmutableSampler :
					break;
				case DST::RayTracingScene :
				/*{
					void* ptr = &mem.EmplaceSized<DS::RayTracingScene>(
										Bytes{sizeof(DS::RayTracingScene) + sizeof(DS::RayTracingScene::Element) * (array_capacity-1)},
										un.index, array_capacity, array_size, Default );

					curr.offset	= uint16_t(mem.OffsetOf( ptr ));
					break;
				}*/
				case DST::Unknown :
				default :
					CHECK(false);
					break;
			}
			END_ENUM_CHECKS();
		}

		std::sort( uniforms_ptr, uniforms_ptr + data->uniformCount,
				   [] (auto& lhs, auto& rhs) { return lhs.name < rhs.name; });

		ASSERT( dbo_count == bufferDynamicOffsetCount );
		return DynamicDataPtr{ data };
	}

}	// AE::Graphics
