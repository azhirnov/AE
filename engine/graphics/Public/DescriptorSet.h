// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "graphics/Public/IDs.h"
#include "graphics/Public/EResourceState.h"
#include "graphics/Public/BufferDesc.h"
#include "graphics/Public/ImageDesc.h"

namespace AE::Graphics
{

	//
	// Descriptor Set Description
	//

	class DescriptorSet
	{
		friend class DescriptorSetHelper;

	// types
	public:
		using Self = DescriptorSet;

		enum class EDescriptorType : uint16_t
		{
			Unknown		= 0,
			UniformBuffer,
			StorageBuffer,
			UniformTexelBuffer,
			StorageTexelBuffer,
			StorageImage,
			SampledImage,					// texture without sampler
			CombinedImage,					// sampled image + sampler
			CombinedImage_ImmutableSampler,	// sampled image + immutable sampler
			SubpassInput,
			Sampler,
			ImmutableSampler,
			RayTracingScene
		};

		using BindingIndex_t	= uint16_t;
		using ArraySize_t		= uint16_t;

		struct Buffer
		{
			struct Element {
				GfxResourceID	bufferId;
				BytesU			offset;
				BytesU			size;
			};

			BindingIndex_t		index				= UMax;
			EResourceState		state				= Default;
			uint				dynamicOffsetIndex	= UMax;
			BytesU				staticSize;
			BytesU				arrayStride;
			ArraySize_t			elementCapacity		= 0;
			ArraySize_t			elementCount		= 0;
			Element				elements[1];
		};
		
		struct TexelBuffer
		{
			struct Element {
				GfxResourceID	bufferId;
				BufferViewDesc	desc;
			};

			BindingIndex_t		index			= UMax;
			EResourceState		state			= Default;
			ArraySize_t			elementCapacity	= 0;
			ArraySize_t			elementCount	= 0;
			Element				elements[1];
		};

		struct Image
		{
			struct Element {
				GfxResourceID	imageId;
				ImageViewDesc	desc;
				bool			hasDesc		= false;
			};

			BindingIndex_t		index			= UMax;
			EResourceState		state			= Default;
			ArraySize_t			elementCapacity	= 0;
			ArraySize_t			elementCount	= 0;
			Element				elements[1];
		};

		struct Texture
		{
			struct Element {
				GfxResourceID	imageId;
				SamplerName		sampler;
				ImageViewDesc	desc;
				bool			hasDesc		= false;
			};
			
			BindingIndex_t		index			= UMax;
			EResourceState		state			= Default;
			ArraySize_t			elementCapacity	= 0;
			ArraySize_t			elementCount	= 0;
			Element				elements[1];
		};

		/*struct SubpassInput : Image
		{
		};*/

		struct Sampler
		{
			struct Element {
				SamplerName		sampler;
			};

			BindingIndex_t		index			= UMax;
			ArraySize_t			elementCapacity	= 0;
			ArraySize_t			elementCount	= 0;
			Element				elements[1];
		};

		/*struct RayTracingScene
		{
			static constexpr EDescriptorType	TypeId = EDescriptorType::RayTracingScene;

			struct Element {
				RawRTSceneID	sceneId;
			};

			BindingIndex_t		index			= UMax;
			ArraySize_t			elementCapacity	= 0;
			ArraySize_t			elementCount	= 0;
			Element				elements[1];
		};*/
		
		struct Uniform
		{
			UniformName			name;
			EDescriptorType		resType		= Default;
			uint16_t			offset		= 0;

			// for sorting and searching
			ND_ bool		operator == (const UniformName &rhs)					const	{ return name == rhs; }
			ND_ bool		operator >  (const UniformName &rhs)					const	{ return name >  rhs; }
			ND_ friend bool	operator >  (const UniformName &lhs, const Uniform &rhs)		{ return lhs  >  rhs.name; }
			ND_ friend bool	operator == (const UniformName &lhs, const Uniform &rhs)		{ return lhs  == rhs.name; }
		};

		struct DynamicData
		{
			DescriptorSetLayoutID	layoutId;
			uint					uniformCount		= 0;
			uint					uniformsOffset		= 0;
			uint					dynamicOffsetsCount	= 0;
			uint					dynamicOffsetsOffset= 0;
			BytesU					memSize;

			DynamicData () {}

			ND_ Uniform	*		Uniforms ()					{ return Cast<Uniform>(this + BytesU{uniformsOffset}); }
			ND_ Uniform	const*	Uniforms ()			const	{ return Cast<Uniform>(this + BytesU{uniformsOffset}); }
			ND_ uint*			DynamicOffsets ()			{ return Cast<uint>(this + BytesU{dynamicOffsetsOffset}); }

			ND_ HashVal			CalcHash () const;
			ND_ bool			operator == (const DynamicData &) const;
				
			template <typename T, typename Fn>	static void  _ForEachUniform (T&&, Fn &&);
			template <typename Fn>				void ForEachUniform (Fn&& fn)				{ _ForEachUniform( *this, fn ); }
			template <typename Fn>				void ForEachUniform (Fn&& fn) const			{ _ForEachUniform( *this, fn ); }
		};
	
		struct DynamicDataDeleter
		{
			constexpr DynamicDataDeleter () {}

			DynamicDataDeleter (const DynamicDataDeleter &) {}

			void operator () (DynamicData *) const;
		};

		using DynamicDataPtr = std::unique_ptr< DynamicData, DynamicDataDeleter >;


	// variables
	private:
		DynamicDataPtr	_dataPtr;
		uint16_t		_index					= UMax;
		bool			_allowEmptyResources	= false;
		bool			_changed				= false;


	// methods
	public:
		DescriptorSet () {}
		DescriptorSet (DescriptorSet &&);
		DescriptorSet (const DescriptorSet &) = delete;
		~DescriptorSet ();

		void operator = (const DescriptorSet &) = delete;
		void operator = (DescriptorSet &&) = delete;

		ND_ DescriptorSet  Clone () const;

		Self&  BindImage (const UniformName &name, GfxResourceID image, uint elementIndex = 0);
		Self&  BindImage (const UniformName &name, GfxResourceID image, const ImageViewDesc &desc, uint elementIndex = 0);
		Self&  BindImages (const UniformName &name, ArrayView<GfxResourceID> images);

		Self&  BindTexture (const UniformName &name, GfxResourceID image, const SamplerName &sampler, uint elementIndex = 0);
		Self&  BindTexture (const UniformName &name, GfxResourceID image, const SamplerName &sampler, const ImageViewDesc &desc, uint elementIndex = 0);
		Self&  BindTextures (const UniformName &name, ArrayView<GfxResourceID> images, const SamplerName &sampler);

		Self&  BindSampler (const UniformName &name, const SamplerName &sampler, uint elementIndex = 0);
		Self&  BindSamplers (const UniformName &name, ArrayView<SamplerName> samplers);

		Self&  BindBuffer (const UniformName &name, GfxResourceID buffer, uint elementIndex = 0);
		Self&  BindBuffer (const UniformName &name, GfxResourceID buffer, BytesU offset, BytesU size, uint elementIndex = 0);
		Self&  BindBuffers (const UniformName &name, ArrayView<GfxResourceID> buffers);
		Self&  SetBufferBase (const UniformName &name, BytesU offset, uint elementIndex = 0);

		Self&  BindTexelBuffer (const UniformName &name, GfxResourceID buffer, const BufferViewDesc &desc, uint elementIndex = 0);

		//Self&  BindRayTracingScene (const UniformName &name, RawRTSceneID scene, uint elementIndex = 0);
		
		void  AllowEmptyResources (bool value)							{ _allowEmptyResources = value; }

		void  Reset (const UniformName &name);
		void  ResetAll ();
		
		ND_ bool  HasImage (const UniformName &name)			const;
		ND_ bool  HasSampler (const UniformName &name)			const;
		ND_ bool  HasTexture (const UniformName &name)			const;
		ND_ bool  HasBuffer (const UniformName &name)			const;
		ND_ bool  HasTexelBuffer (const UniformName &name)		const;
		ND_ bool  HasRayTracingScene (const UniformName &name)	const;

		ND_ EDescriptorType	GetType (const UniformName &name)	const;
		
		ND_ DescriptorSetLayoutID	GetLayout ()				const	{ ASSERT(_dataPtr);  return _dataPtr->layoutId; }
		ND_ ArrayView< uint >		GetDynamicOffsets ()		const;
		ND_ bool					IsEmptyResourcesAllowed ()	const	{ return _allowEmptyResources; }
		ND_ bool					IsInitialized ()			const	{ return _dataPtr != null; }
		ND_ bool					IsChanged ()				const	{ return _changed; }
		ND_ uint					Index ()					const	{ return _index; }

	private:
		ND_ uint &					_GetDynamicOffset (uint i)		{ ASSERT( _dataPtr and i < _dataPtr->dynamicOffsetsCount );  return _dataPtr->DynamicOffsets()[i]; }

		template <typename T> T *	_GetResource (const UniformName &name);
		template <typename T> bool	_HasResource (const UniformName &name) const;

		template <typename T> static bool  _CheckResourceType (EDescriptorType type);
	};
	

/*
=================================================
	GetDynamicOffsets
=================================================
*/
	inline ArrayView<uint>  DescriptorSet::GetDynamicOffsets () const
	{
		return _dataPtr and _dataPtr->dynamicOffsetsCount ? ArrayView<uint>{ _dataPtr->DynamicOffsets(), _dataPtr->dynamicOffsetsCount } : ArrayView<uint>{};
	}
	
/*
=================================================
	_ForEachUniform
=================================================
*/
	template <typename T, typename Fn>
	inline void  DescriptorSet::DynamicData::_ForEachUniform (T&& self, Fn&& fn)
	{
		STATIC_ASSERT( IsSameTypes< std::remove_cv_t<std::remove_reference_t<T>>, DynamicData > );

		for (uint i = 0, cnt = self.uniformCount; i < cnt; ++i)
		{
			auto&	un  = self.Uniforms()[i];
			auto*	ptr = (&self + BytesU{un.offset});

			BEGIN_ENUM_CHECKS();
			switch ( un.resType )
			{
				case EDescriptorType::Unknown :							break;
				case EDescriptorType::UniformBuffer :
				case EDescriptorType::StorageBuffer :					fn( un.name, un.resType, *Cast<Buffer>(ptr) );			break;
				case EDescriptorType::UniformTexelBuffer :
				case EDescriptorType::StorageTexelBuffer :				fn( un.name, un.resType, *Cast<TexelBuffer>(ptr) );		break;
				case EDescriptorType::SubpassInput :
				case EDescriptorType::StorageImage :
				case EDescriptorType::SampledImage :
				case EDescriptorType::CombinedImage_ImmutableSampler :	fn( un.name, un.resType, *Cast<Image>(ptr) );			break;
				case EDescriptorType::CombinedImage :					fn( un.name, un.resType, *Cast<Texture>(ptr) );			break;
				case EDescriptorType::Sampler :							fn( un.name, un.resType, *Cast<Sampler>(ptr) );			break;
				case EDescriptorType::RayTracingScene :					//fn( un.name, un.resType, *Cast<RayTracingScene>(ptr) );	break;
				case EDescriptorType::ImmutableSampler :
				default :												CHECK(false);	break;
			}
			END_ENUM_CHECKS();
		}
	}


}	// AE::Graphics
