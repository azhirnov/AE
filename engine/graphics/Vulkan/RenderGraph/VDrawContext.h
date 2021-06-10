// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN
# include "graphics/Public/CommandBuffer.h"
# include "graphics/Vulkan/VRenderGraph.h"
# include "graphics/Vulkan/VEnumCast.h"

namespace AE::Graphics::_hidden_
{

	//
	// Vulkan Direct Draw Context implementation
	//
	
	template <typename ResMngrType>
	class _VDirectDrawCtx : protected VulkanDeviceFn
	{
	// types
	public:
		static constexpr bool	IsIndirectContext	= false;
		static constexpr bool	HasAutoSync			= ResMngrType::HasAutoSync;
		
	// variables
	protected:
		VCommandBuffer		_cmdbuf;

		// cached states
		struct {
			VkPipelineLayout	pplnLayout		= VK_NULL_HANDLE;
			VkPipeline			pipeline		= VK_NULL_HANDLE;

			VkBuffer			indexBuffer		= VK_NULL_HANDLE;
			Bytes				indexOffset;
		}					_states;

		ResMngrType			_resMngr;


	// methods
	public:
		explicit _VDirectDrawCtx (const VCommandBatch& batch);
		virtual ~_VDirectDrawCtx ();

		ND_ bool			IsValid ()				const	{ return _cmdbuf.IsValid(); }
		ND_ VkCommandBuffer	ReleaseCommandBuffer ();

		void  BindDescriptorSet (uint index, VkDescriptorSet ds, ArrayView<uint> dynamicOffsets);

		void  BindIndexBuffer (VkBuffer buffer, Bytes offset, VkIndexType indexType);

		void  DrawIndirect (VkBuffer	indirectBuffer,
							Bytes		indirectBufferOffset,
							uint		drawCount,
							Bytes		stride);

		void  DrawIndexedIndirect (VkBuffer		indirectBuffer,
								   Bytes		indirectBufferOffset,
								   uint			drawCount,
								   Bytes		stride);
		
		void  DrawIndirectCount (VkBuffer	indirectBuffer,
								 Bytes		indirectBufferOffset,
								 VkBuffer	countBuffer,
								 Bytes		countBufferOffset,
								 uint		maxDrawCount,
								 Bytes		stride);
		
		void  DrawIndexedIndirectCount (VkBuffer	indirectBuffer,
										Bytes		indirectBufferOffset,
										VkBuffer	countBuffer,
										Bytes		countBufferOffset,
										uint		maxDrawCount,
										Bytes		stride);
		
		void  DrawMeshTasksIndirectNV (VkBuffer		indirectBuffer,
									   Bytes		indirectBufferOffset,
									   uint			drawCount,
									   Bytes		stride);
		
		void  DrawMeshTasksIndirectCountNV (VkBuffer	indirectBuffer,
											Bytes		indirectBufferOffset,
											VkBuffer	countBuffer,
											Bytes		countBufferOffset,
											uint		maxDrawCount,
											Bytes		stride);

	protected:
		void  _ResetStates ();

		void  _BindGraphicsPipeline (VkPipeline ppln, VkPipelineLayout layout);
		void  _PushGraphicsConstant (Bytes offset, Bytes size, const void *values, VkShaderStageFlags stages);

		void  _SetScissor (uint first, ArrayView<VkRect2D> scissors);
		void  _SetDepthBias (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
		void  _SetDepthBounds (float minDepthBounds, float maxDepthBounds);
		void  _SetStencilCompareMask (VkStencilFaceFlagBits faceMask, uint compareMask);
		void  _SetStencilWriteMask (VkStencilFaceFlagBits faceMask, uint writeMask);
		void  _SetStencilReference (VkStencilFaceFlagBits faceMask, uint reference);
		void  _SetBlendConstants (const RGBA32f &color);
		
		void  _BindVertexBuffers (uint firstBinding, ArrayView<VkBuffer> buffers, ArrayView<VkDeviceSize> offsets);

		void  _Draw (uint vertexCount,
					 uint instanceCount,
					 uint firstVertex,
					 uint firstInstance);

		void  _DrawIndexed (uint indexCount,
							uint instanceCount,
							uint firstIndex,
							int  vertexOffset,
							uint firstInstance);

		void  _DrawMeshTasksNV (uint taskCount, uint firstTask);
	};

	

	//
	// Vulkan Indirect Draw Context implementation
	//
	
	class _VBaseIndirectDrawCtx
	{
	// types
	public:
		static constexpr bool	IsIndirectContext	= true;
		
	protected:
		struct BaseCmd
		{
			DEBUG_ONLY(
				static constexpr uint	MAGIC = 0x762a3cf0;
				uint	_magicNumber = MAGIC;
			)
			ushort	_commandID	= 0;
			ushort	_size		= 0;
		};

		struct BindPipelineCmd : BaseCmd
		{
			VkPipeline			ppln;
			VkPipelineLayout	layout;
		};

		struct BindDescriptorSetCmd : BaseCmd
		{
			VkDescriptorSet	ds;
			ushort			index;
			ushort			dynamicOffsetsCount;
			//uint			dynamicOffsets[];
		};

		struct PushConstantCmd : BaseCmd
		{
			VkShaderStageFlags	stages;
			ushort				offset;
			ushort				size;
			//uint				data[];
		};

		struct SetScissorCmd : BaseCmd
		{
			ushort		first;
			ushort		count;
			//VkRect2D	scissors[];
		};

		struct SetDepthBiasCmd : BaseCmd
		{
			float	depthBiasConstantFactor;
			float	depthBiasClamp;
			float	depthBiasSlopeFactor;
		};

		struct SetDepthBoundsCmd : BaseCmd
		{
			float	minDepthBounds;
			float	maxDepthBounds;
		};

		struct SetStencilCompareMaskCmd : BaseCmd
		{
			VkStencilFaceFlagBits	faceMask;
			uint					compareMask;
		};

		struct SetStencilWriteMaskCmd : BaseCmd
		{
			VkStencilFaceFlagBits	faceMask;
			uint					writeMask;
		};

		struct SetStencilReferenceCmd : BaseCmd
		{
			VkStencilFaceFlagBits	faceMask;
			uint					reference;
		};

		struct SetBlendConstantsCmd : BaseCmd
		{
			RGBA32f		color;
		};

		struct BindIndexBufferCmd : BaseCmd
		{
			VkBuffer		buffer;
			VkDeviceSize	offset;
			VkIndexType		indexType;
		};

		struct BindVertexBuffersCmd : BaseCmd
		{
			ushort			firstBinding;
			ushort			count;
			//VkBuffer		buffers[];
			//VkDeviceSize	offsets[];
		};

		struct DrawCmd : BaseCmd
		{
			uint	vertexCount;
			uint	instanceCount;
			uint	firstVertex;
			uint	firstInstance;
		};

		struct DrawIndexedCmd : BaseCmd
		{
			uint	indexCount;
			uint	instanceCount;
			uint	firstIndex;
			int		vertexOffset;
			uint	firstInstance;
		};

		struct DrawIndirectCmd : BaseCmd
		{
			VkBuffer		indirectBuffer;
			VkDeviceSize	indirectBufferOffset;
			uint			drawCount;
			uint			stride;
		};
		
		struct DrawIndexedIndirectCmd : BaseCmd
		{
			VkBuffer		indirectBuffer;
			VkDeviceSize	indirectBufferOffset;
			uint			drawCount;
			uint			stride;
		};
		
		struct DrawIndirectCountCmd : BaseCmd
		{
			VkBuffer		indirectBuffer;
			VkDeviceSize	indirectBufferOffset;
			VkBuffer		countBuffer;
			VkDeviceSize	countBufferOffset;
			uint			maxDrawCount;
			uint			stride;
		};

		struct DrawIndexedIndirectCountCmd : BaseCmd
		{
			VkBuffer		indirectBuffer;
			VkDeviceSize	indirectBufferOffset;
			VkBuffer		countBuffer;
			VkDeviceSize	countBufferOffset;
			uint			maxDrawCount;
			uint			stride;
		};

		struct DrawMeshTasksNVCmd : BaseCmd
		{
			uint	taskCount;
			uint	firstTask;
		};

		struct DrawMeshTasksIndirectNVCmd : BaseCmd
		{
			VkBuffer		indirectBuffer;
			VkDeviceSize	indirectBufferOffset;
			uint			drawCount;
			uint			stride;
		};
		
		struct DrawMeshTasksIndirectCountNVCmd : BaseCmd
		{
			VkBuffer		indirectBuffer;
			VkDeviceSize	indirectBufferOffset;
			VkBuffer		countBuffer;
			VkDeviceSize	countBufferOffset;
			uint			maxDrawCount;
			uint			stride;
		};

		using AllCommands_t = TypeList< BindPipelineCmd, BindDescriptorSetCmd, PushConstantCmd, SetScissorCmd, SetDepthBiasCmd,
										SetDepthBoundsCmd, SetStencilCompareMaskCmd, SetStencilWriteMaskCmd, SetStencilReferenceCmd,
										SetBlendConstantsCmd, BindIndexBufferCmd, BindVertexBuffersCmd, DrawCmd, DrawIndexedCmd,
										DrawIndirectCmd, DrawIndexedIndirectCmd, DrawIndirectCountCmd, DrawIndexedIndirectCountCmd,
										DrawMeshTasksNVCmd, DrawMeshTasksIndirectNVCmd, DrawMeshTasksIndirectCountNVCmd >;
		
		static constexpr usize	_MaxCmdAlign	= AllCommands_t::ForEach_Max< TypeListUtils::GetTypeAlign >();
		STATIC_ASSERT( VBakedCommands::Allocator_t::Align >= _MaxCmdAlign );


	// variables
	protected:
		LinearAllocator<>	_allocator;
		
		// cached states
		struct {
			VkPipelineLayout	pplnLayout		= VK_NULL_HANDLE;
			VkPipeline			pipeline		= VK_NULL_HANDLE;

			VkBuffer			indexBuffer		= VK_NULL_HANDLE;
			VkDeviceSize		indexOffset		= 0;
		}					_states;


	// methods
	public:
		explicit _VBaseIndirectDrawCtx (const VCommandBatch& batch);
		virtual ~_VBaseIndirectDrawCtx () {}
		
		ND_ bool			IsValid ()	const	{ return true; }
		ND_ VBakedCommands	Prepare ();

		void  BindDescriptorSet (uint index, VkDescriptorSet ds, ArrayView<uint> dynamicOffsets);

		void  BindIndexBuffer (VkBuffer buffer, Bytes offset, VkIndexType indexType);

		void  DrawIndirect (VkBuffer	indirectBuffer,
							Bytes		indirectBufferOffset,
							uint		drawCount,
							Bytes		stride);

		void  DrawIndexedIndirect (VkBuffer		indirectBuffer,
								   Bytes		indirectBufferOffset,
								   uint			drawCount,
								   Bytes		stride);
		
		void  DrawIndirectCount (VkBuffer	indirectBuffer,
								 Bytes		indirectBufferOffset,
								 VkBuffer	countBuffer,
								 Bytes		countBufferOffset,
								 uint		maxDrawCount,
								 Bytes		stride);
		
		void  DrawIndexedIndirectCount (VkBuffer	indirectBuffer,
										Bytes		indirectBufferOffset,
										VkBuffer	countBuffer,
										Bytes		countBufferOffset,
										uint		maxDrawCount,
										Bytes		stride);
		
		void  DrawMeshTasksIndirectNV (VkBuffer		indirectBuffer,
									   Bytes		indirectBufferOffset,
									   uint			drawCount,
									   Bytes		stride);
		
		void  DrawMeshTasksIndirectCountNV (VkBuffer	indirectBuffer,
											Bytes		indirectBufferOffset,
											VkBuffer	countBuffer,
											Bytes		countBufferOffset,
											uint		maxDrawCount,
											Bytes		stride);

	protected:
		void  _ResetStates ();

		void  _BindGraphicsPipeline (VkPipeline ppln, VkPipelineLayout layout);
		void  _PushGraphicsConstant (Bytes offset, Bytes size, const void *values, VkShaderStageFlags stages);

		void  _SetScissor (uint first, ArrayView<VkRect2D> scissors);
		void  _SetDepthBias (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor);
		void  _SetDepthBounds (float minDepthBounds, float maxDepthBounds);
		void  _SetStencilCompareMask (VkStencilFaceFlagBits faceMask, uint compareMask);
		void  _SetStencilWriteMask (VkStencilFaceFlagBits faceMask, uint writeMask);
		void  _SetStencilReference (VkStencilFaceFlagBits faceMask, uint reference);
		void  _SetBlendConstants (const RGBA32f &color);
		
		void  _BindVertexBuffers (uint firstBinding, ArrayView<VkBuffer> buffers, ArrayView<VkDeviceSize> offsets);

		void  _Draw (uint vertexCount,
					 uint instanceCount,
					 uint firstVertex,
					 uint firstInstance);

		void  _DrawIndexed (uint indexCount,
							uint instanceCount,
							uint firstIndex,
							int  vertexOffset,
							uint firstInstance);

		void  _DrawMeshTasksNV (uint taskCount, uint firstTask);


	private:
		template <typename CmdType, typename ...DynamicTypes>
		ND_ CmdType&  _CreateCmd (usize dynamicArraySize = 0);
		
		template <usize I, typename TL>
		static constexpr Bytes  _CalcCmdSize (Bytes size, usize dynamicArraySize);

		static bool  _Execute (VulkanDeviceFn fn, VkCommandBuffer cmdbuf, void* root);
	};
	

	template <typename ResMngrType, typename BaseCtx>
	class _VDrawCtxHelper : public BaseCtx
	{
	// types
	public:
		static constexpr bool	HasAutoSync	= ResMngrType::HasAutoSync;

	// variables
	protected:
		ResMngrType		_resMngr;
	
	// methods
	public:
		explicit _VDrawCtxHelper (const VCommandBatch& batch) : BaseCtx{batch}, _resMngr{batch} {}
	};



	//
	// Vulkan Draw Context implementation
	//

	template <typename CtxImpl>
	class _VDrawContextImpl : public CtxImpl, public IDrawContext
	{
	// types
	public:
		static constexpr bool	IsRenderContext			= true;
		static constexpr bool	IsVulkanRenderContext	= true;
		
	private:
		using RawCtx	= CtxImpl;


	// methods
	public:
		explicit _VDrawContextImpl (const VCommandBatch& batch);
		_VDrawContextImpl () = delete;
		_VDrawContextImpl (const _VDrawContextImpl &) = delete;
		~_VDrawContextImpl ();
		
		// reset graphics pipeline, descriptor sets, push constants and all dynamic render states.
		void  ResetStates () override final;

		// pipeline and shader resources
		void  BindPipeline (GraphicsPipelineID ppln) override final;
		void  BindPipeline (MeshPipelineID ppln) override final;

		using RawCtx::BindDescriptorSet;

		void  BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets) override final;
		void  PushConstant (Bytes offset, Bytes size, const void *values, EShaderStages stages) override final;
		
		// dynamic states
		void  SetScissor (uint first, ArrayView<RectI> scissors) override final;
		void  SetDepthBias (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) override final;
		void  SetDepthBounds (float minDepthBounds, float maxDepthBounds) override final;
		void  SetStencilCompareMask (EStencilFace faceMask, uint compareMask) override final;
		void  SetStencilWriteMask (EStencilFace faceMask, uint writeMask) override final;
		void  SetStencilReference (EStencilFace faceMask, uint reference) override final;
		void  SetBlendConstants (const RGBA32f &color) override final;
		
		// draw commands
		using RawCtx::BindIndexBuffer;

		void  BindIndexBuffer (BufferID buffer, Bytes offset, EIndex indexType) override final;
		void  BindVertexBuffer (uint index, BufferID buffer, Bytes offset) override final;
		void  BindVertexBuffer (uint index, VkBuffer buffer, Bytes offset);
		void  BindVertexBuffers (uint firstBinding, ArrayView<Pair<BufferID, Bytes>> bufferAndOffset) override final;
		void  BindVertexBuffers (uint firstBinding, ArrayView<Pair<VkBuffer, Bytes>> bufferAndOffset);

		using RawCtx::DrawIndirect;
		using RawCtx::DrawIndexedIndirect;

		void  Draw (uint vertexCount,
					uint instanceCount,
					uint firstVertex,
					uint firstInstance) override final;

		void  DrawIndexed (uint indexCount,
						   uint instanceCount,
						   uint firstIndex,
						   int  vertexOffset,
						   uint firstInstance) override final;

		void  DrawIndirect (BufferID	indirectBuffer,
							Bytes		indirectBufferOffset,
							uint		drawCount,
							Bytes		stride) override final;

		void  DrawIndexedIndirect (BufferID		indirectBuffer,
								   Bytes		indirectBufferOffset,
								   uint			drawCount,
								   Bytes		stride) override final;

		// extension
		using RawCtx::DrawIndirectCount;
		using RawCtx::DrawIndexedIndirectCount;

		void  DrawIndirectCount (BufferID	indirectBuffer,
								 Bytes		indirectBufferOffset,
								 BufferID	countBuffer,
								 Bytes		countBufferOffset,
								 uint		maxDrawCount,
								 Bytes		stride);

		void  DrawIndexedIndirectCount (BufferID	indirectBuffer,
										Bytes		indirectBufferOffset,
										BufferID	countBuffer,
										Bytes		countBufferOffset,
										uint		maxDrawCount,
										Bytes		stride);

		// mesh draw commands (extension)
		using RawCtx::DrawMeshTasksIndirectNV;
		using RawCtx::DrawMeshTasksIndirectCountNV;

		void  DrawMeshTasksNV (uint taskCount, uint firstTask);

		void  DrawMeshTasksIndirectNV (BufferID		indirectBuffer,
									   Bytes		indirectBufferOffset,
									   uint			drawCount,
									   Bytes		stride);

		void  DrawMeshTasksIndirectCountNV (BufferID	indirectBuffer,
											Bytes		indirectBufferOffset,
											BufferID	countBuffer,
											Bytes		countBufferOffset,
											uint		maxDrawCount,
											Bytes		stride);

	private:
		ND_ auto&  GetFeatures ()	const	{ return this->_resMngr.GetDevice().GetFeatures(); }
	};
//-----------------------------------------------------------------------------
	
	

/*
=================================================
	ResetStates
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::ResetStates ()
	{
		RawCtx::_ResetStates();
	}
	
/*
=================================================
	BindPipeline
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::BindPipeline (GraphicsPipelineID ppln)
	{
		auto*	gppln = this->_resMngr.Get( ppln );
		CHECK_ERRV( gppln );

		RawCtx::_BindGraphicsPipeline( gppln->Handle(), gppln->Layout() );
	}
	
/*
=================================================
	BindPipeline
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::BindPipeline (MeshPipelineID ppln)
	{
		auto*	mppln = this->_resMngr.Get( ppln );
		CHECK_ERRV( mppln );
		
		RawCtx::_BindGraphicsPipeline( mppln->Handle(), mppln->Layout() );
	}
	
/*
=================================================
	BindDescriptorSet
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::BindDescriptorSet (uint index, DescriptorSetID ds, ArrayView<uint> dynamicOffsets)
	{
		auto*	desc_set = this->_resMngr.Get( ds );
		CHECK_ERRV( desc_set );

		BindDescriptorSet( index, desc_set->Handle(), dynamicOffsets );
	}
	
/*
=================================================
	PushConstant
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::PushConstant (Bytes offset, Bytes size, const void *values, EShaderStages stages)
	{
		ASSERT( IsAligned( size, sizeof(uint) ));

		RawCtx::_PushGraphicsConstant( offset, size, values, VEnumCast(stages) );
	}
		
/*
=================================================
	SetScissor
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::SetScissor (uint first, ArrayView<RectI> scissors)
	{
		ASSERT( scissors.size() );

		StaticArray< VkRect2D, GraphicsConfig::MaxViewports >	vk_scissors;

		for (usize i = 0; i < scissors.size(); ++i)
		{
			auto&	src = scissors[i];
			auto&	dst = vk_scissors[i];

			dst.offset	= { src.left,          src.top };
			dst.extent	= { uint(src.Width()), uint(src.Height()) };
		}
		RawCtx::_SetScissor( first, ArrayView<VkRect2D>{ vk_scissors.data(), scissors.size() });
	}
	
/*
=================================================
	SetDepthBias
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::SetDepthBias (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
	{
		RawCtx::_SetDepthBias( depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor );
	}
	
/*
=================================================
	SetDepthBounds
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::SetDepthBounds (float minDepthBounds, float maxDepthBounds)
	{
		RawCtx::_SetDepthBounds( minDepthBounds, maxDepthBounds );
	}
	
/*
=================================================
	SetStencilCompareMask
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::SetStencilCompareMask (EStencilFace faceMask, uint compareMask)
	{
		RawCtx::_SetStencilCompareMask( VEnumCast(faceMask), compareMask );
	}
	
/*
=================================================
	SetStencilWriteMask
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::SetStencilWriteMask (EStencilFace faceMask, uint writeMask)
	{
		RawCtx::_SetStencilWriteMask( VEnumCast(faceMask), writeMask );
	}
	
/*
=================================================
	SetStencilReference
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::SetStencilReference (EStencilFace faceMask, uint reference)
	{
		RawCtx::_SetStencilReference( VEnumCast(faceMask), reference );
	}
	
/*
=================================================
	SetBlendConstants
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::SetBlendConstants (const RGBA32f &color)
	{
		RawCtx::_SetBlendConstants( color );
	}
	
/*
=================================================
	BindIndexBuffer
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::BindIndexBuffer (BufferID buffer, Bytes offset, EIndex indexType)
	{
		auto*	buf = this->_resMngr.Get( buffer );
		CHECK_ERRV( buf );

		return BindIndexBuffer( buf->Handle(), offset, VEnumCast(indexType) );
	}
	
/*
=================================================
	BindVertexBuffer
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::BindVertexBuffer (uint index, BufferID buffer, Bytes offset)
	{
		auto*	buf = this->_resMngr.Get( buffer );
		CHECK_ERRV( buf );

		BindVertexBuffer( index, buf->Handle(), offset );
	}
	
	template <typename C>
	void  _VDrawContextImpl<C>::BindVertexBuffer (uint index, VkBuffer buffer, Bytes offset)
	{
		const VkDeviceSize	off	= VkDeviceSize(offset);
		RawCtx::_BindVertexBuffers( index, ArrayView<VkBuffer>{&buffer, 1}, ArrayView<VkDeviceSize>{&off, 1} );
	}
	
/*
=================================================
	BindVertexBuffers
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::BindVertexBuffers (uint firstBinding, ArrayView<Pair<BufferID, Bytes>> bufferAndOffset)
	{
		ASSERT( bufferAndOffset.size() <= GraphicsConfig::MaxVertexBuffers );

		StaticArray< VkBuffer, GraphicsConfig::MaxVertexBuffers >		buffers;
		StaticArray< VkDeviceSize, GraphicsConfig::MaxVertexBuffers >	offsets;

		for (uint i = 0; i < bufferAndOffset.size(); ++i)
		{
			auto&	off		= bufferAndOffset[i].second;
			auto*	buffer	= this->_resMngr.Get( bufferAndOffset[i].first );
			CHECK_ERRV( buffer );

			buffers[i]	= buffer->Handle();
			offsets[i]	= VkDeviceSize(off);
		}

		RawCtx::_BindVertexBuffers( firstBinding, ArrayView<VkBuffer>{ buffers.data(), bufferAndOffset.size() }, ArrayView<VkDeviceSize>{ offsets.data(), bufferAndOffset.size() });
	}
	
	template <typename C>
	void  _VDrawContextImpl<C>::BindVertexBuffers (uint firstBinding, ArrayView<Pair<VkBuffer, Bytes>> bufferAndOffset)
	{
		ASSERT( bufferAndOffset.size() <= GraphicsConfig::MaxVertexBuffers );

		StaticArray< VkBuffer, GraphicsConfig::MaxVertexBuffers >		buffers;
		StaticArray< VkDeviceSize, GraphicsConfig::MaxVertexBuffers >	offsets;

		for (uint i = 0; i < bufferAndOffset.size(); ++i)
		{
			buffers[i]	= bufferAndOffset[i].first;
			offsets[i]	= VkDeviceSize(bufferAndOffset[i].second);
		}
		
		RawCtx::_BindVertexBuffers( firstBinding, ArrayView<VkBuffer>{ buffers.data(), bufferAndOffset.size() }, ArrayView<VkDeviceSize>{ offsets.data(), bufferAndOffset.size() });
	}

/*
=================================================
	Draw
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::Draw (uint vertexCount,
									  uint instanceCount,
									  uint firstVertex,
									  uint firstInstance)
	{
		RawCtx::_Draw( vertexCount, instanceCount, firstVertex, firstInstance );
	}
	
/*
=================================================
	DrawIndexed
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::DrawIndexed (uint indexCount,
											 uint instanceCount,
											 uint firstIndex,
											 int  vertexOffset,
											 uint firstInstance)
	{
		RawCtx::_DrawIndexed( indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
	}
	
/*
=================================================
	DrawIndirect
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::DrawIndirect (BufferID	indirectBuffer,
											  Bytes		indirectBufferOffset,
											  uint		drawCount,
											  Bytes		stride)
	{
		auto*	buf = this->_resMngr.Get( indirectBuffer );
		CHECK_ERRV( buf );

		DrawIndirect( buf->Handle(), indirectBufferOffset, drawCount, stride );
	}
	
/*
=================================================
	DrawIndexedIndirect
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::DrawIndexedIndirect (BufferID	indirectBuffer,
													 Bytes		indirectBufferOffset,
													 uint		drawCount,
													 Bytes		stride)
	{
		auto*	buf = this->_resMngr.Get( indirectBuffer );
		CHECK_ERRV( buf );

		RawCtx::DrawIndexedIndirect( buf->Handle(), indirectBufferOffset, drawCount, stride );
	}
	
/*
=================================================
	DrawIndirectCount
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::DrawIndirectCount (BufferID	indirectBuffer,
													   Bytes	indirectBufferOffset,
													   BufferID	countBuffer,
													   Bytes	countBufferOffset,
													   uint		maxDrawCount,
													   Bytes	stride)
	{
		CHECK_ERRV( GetFeatures().drawIndirectCount );

		auto*	ibuf = this->_resMngr.Get( indirectBuffer );
		auto*	cbuf = this->_resMngr.Get( countBuffer );
		CHECK_ERRV( ibuf and cbuf );

		RawCtx::DrawIndirectCount( ibuf->Handle(), indirectBufferOffset, cbuf->Handle(), countBufferOffset, maxDrawCount, stride );
	}
	
/*
=================================================
	DrawIndexedIndirectCount
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::DrawIndexedIndirectCount (BufferID	indirectBuffer,
															  Bytes		indirectBufferOffset,
															  BufferID	countBuffer,
															  Bytes		countBufferOffset,
															  uint		maxDrawCount,
															  Bytes		stride)
	{
		CHECK_ERRV( GetFeatures().drawIndirectCount );

		auto*	ibuf = this->_resMngr.Get( indirectBuffer );
		auto*	cbuf = this->_resMngr.Get( countBuffer );
		CHECK_ERRV( ibuf and cbuf );
		
		RawCtx::DrawIndexedIndirectCount( ibuf->Handle(), indirectBufferOffset, cbuf->Handle(), countBufferOffset, maxDrawCount, stride );
	}
	
/*
=================================================
	DrawMeshTasksNV
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::DrawMeshTasksNV (uint taskCount, uint firstTask)
	{
	#ifdef VK_NV_mesh_shader
		CHECK_ERRV( GetFeatures().meshShaderNV );
		
		RawCtx::_DrawMeshTasksNV( taskCount, firstTask );

	#else
		Unused( taskCount, firstTask );
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}
	
/*
=================================================
	DrawMeshTasksIndirectNV
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::DrawMeshTasksIndirectNV (BufferID	indirectBuffer,
														 Bytes		indirectBufferOffset,
														 uint		drawCount,
														 Bytes		stride)
	{
	#ifdef VK_NV_mesh_shader
		CHECK_ERRV( GetFeatures().meshShaderNV );
		
		auto*	buf = this->_resMngr.Get( indirectBuffer );
		CHECK_ERRV( buf );

		RawCtx::DrawMeshTasksIndirectNV( buf->Handle(), indirectBufferOffset, drawCount, stride );

	#else
		Unused( indirectBuffer, indirectBufferOffset, drawCount, stride );
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}
	
/*
=================================================
	DrawMeshTasksIndirectCountNV
=================================================
*/
	template <typename C>
	void  _VDrawContextImpl<C>::DrawMeshTasksIndirectCountNV (BufferID	indirectBuffer,
															  Bytes		indirectBufferOffset,
															  BufferID	countBuffer,
															  Bytes		countBufferOffset,
															  uint		maxDrawCount,
															  Bytes		stride)
	{
	#ifdef VK_NV_mesh_shader
		CHECK_ERRV( GetFeatures().meshShaderNV );
		
		auto*	ibuf = this->_resMngr.Get( indirectBuffer );
		auto*	cbuf = this->_resMngr.Get( countBuffer );
		CHECK_ERRV( ibuf and cbuf );
		
		RawCtx::DrawMeshTasksIndirectCountNV( ibuf->Handle(), indirectBufferOffset, cbuf->Handle(), countBufferOffset, maxDrawCount, stride );

	#else
		Unused( indirectBuffer, indirectBufferOffset, countBuffer, countBufferOffset, maxDrawCount, stride );
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	template <typename RM>
	_VDirectDrawCtx<RM>::_VDirectDrawCtx (const VCommandBatch& batch) :
		_cmdbuf{ RenderGraph().GetCommandPoolManager().GetCommandBuffer( batch.GetQueueType(), ECommandBufferType::Secondary_RenderCommands )},
		_resMngr{ batch }
	{
		VulkanDeviceFn_Init( _resMngr.GetDevice() );
	}
	
/*
=================================================
	destructor
=================================================
*/
	template <typename RM>
	_VDirectDrawCtx<RM>::~_VDirectDrawCtx ()
	{
		ASSERT( not _cmdbuf.IsValid() and "you forget to call 'ReleaseCommandBuffer()'" );
	}
	
/*
=================================================
	_ResetStates
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_ResetStates ()
	{
		_states.pplnLayout	= VK_NULL_HANDLE;
		_states.pipeline	= VK_NULL_HANDLE;

		_states.indexBuffer	= VK_NULL_HANDLE;
		_states.indexOffset	= 0_b;
	}
	
/*
=================================================
	_BindGraphicsPipeline
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_BindGraphicsPipeline (VkPipeline ppln, VkPipelineLayout layout)
	{
		if ( _states.pipeline == ppln )
			return;

		_states.pipeline	= ppln;
		_states.pplnLayout	= layout;
		vkCmdBindPipeline( _cmdbuf.Get(), VK_PIPELINE_BIND_POINT_GRAPHICS, _states.pipeline );
	}
	
/*
=================================================
	BindDescriptorSet
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::BindDescriptorSet (uint index, VkDescriptorSet ds, ArrayView<uint> dynamicOffsets)
	{
		CHECK_ERRV( _states.pplnLayout );
		vkCmdBindDescriptorSets( _cmdbuf.Get(), VK_PIPELINE_BIND_POINT_GRAPHICS, _states.pplnLayout, index, 1, &ds, uint(dynamicOffsets.size()), dynamicOffsets.data() );
	}
	
/*
=================================================
	_PushGraphicsConstant
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_PushGraphicsConstant (Bytes offset, Bytes size, const void *values, VkShaderStageFlags stages)
	{
		ASSERT( IsAligned( size, sizeof(uint) ));
		ASSERT( _states.pplnLayout );

		vkCmdPushConstants( _cmdbuf.Get(), _states.pplnLayout, stages, uint(offset), uint(size), values );
	}
	
/*
=================================================
	_SetScissor
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_SetScissor (uint first, ArrayView<VkRect2D> scissors)
	{
		ASSERT( scissors.size() );
		vkCmdSetScissor( _cmdbuf.Get(), first, uint(scissors.size()), scissors.data() );
	}
	
/*
=================================================
	_SetDepthBias
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_SetDepthBias (float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
	{
		vkCmdSetDepthBias( _cmdbuf.Get(), depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor );
	}
	
/*
=================================================
	_SetDepthBounds
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_SetDepthBounds (float minDepthBounds, float maxDepthBounds)
	{
		vkCmdSetDepthBounds( _cmdbuf.Get(), minDepthBounds, maxDepthBounds );
	}
	
/*
=================================================
	_SetStencilCompareMask
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_SetStencilCompareMask (VkStencilFaceFlagBits faceMask, uint compareMask)
	{
		vkCmdSetStencilCompareMask( _cmdbuf.Get(), faceMask, compareMask );
	}
	
/*
=================================================
	_SetStencilWriteMask
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_SetStencilWriteMask (VkStencilFaceFlagBits faceMask, uint writeMask)
	{
		vkCmdSetStencilWriteMask( _cmdbuf.Get(), faceMask, writeMask );
	}
	
/*
=================================================
	_SetStencilReference
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_SetStencilReference (VkStencilFaceFlagBits faceMask, uint reference)
	{
		vkCmdSetStencilReference( _cmdbuf.Get(), faceMask, reference );
	}
	
/*
=================================================
	_SetBlendConstants
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_SetBlendConstants (const RGBA32f &color)
	{
		vkCmdSetBlendConstants( _cmdbuf.Get(), color.data() );
	}

/*
=================================================
	BindIndexBuffer
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::BindIndexBuffer (VkBuffer buffer, Bytes offset, VkIndexType indexType)
	{
		if ( (_states.indexBuffer == buffer) & (_states.indexOffset == offset) )
			return;

		_states.indexBuffer	= buffer;
		_states.indexOffset	= offset;
		vkCmdBindIndexBuffer( _cmdbuf.Get(), _states.indexBuffer, VkDeviceSize(_states.indexOffset), indexType );
	}
	
/*
=================================================
	_BindVertexBuffers
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_BindVertexBuffers (uint firstBinding, ArrayView<VkBuffer> buffers, ArrayView<VkDeviceSize> offsets)
	{
		ASSERT( buffers.size() );
		ASSERT( buffers.size() == offsets.size() );

		vkCmdBindVertexBuffers( _cmdbuf.Get(), firstBinding, uint(buffers.size()), buffers.data(), offsets.data() );
	}
	
/*
=================================================
	_Draw
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_Draw (uint vertexCount,
									  uint instanceCount,
									  uint firstVertex,
									  uint firstInstance)
	{
		vkCmdDraw( _cmdbuf.Get(), vertexCount, instanceCount, firstVertex, firstInstance );
	}
	
/*
=================================================
	_DrawIndexed
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_DrawIndexed (uint indexCount,
											 uint instanceCount,
											 uint firstIndex,
											 int  vertexOffset,
											 uint firstInstance)
	{
		vkCmdDrawIndexed( _cmdbuf.Get(), indexCount, instanceCount, firstIndex, vertexOffset, firstInstance );
	}
	
/*
=================================================
	DrawIndirect
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::DrawIndirect (VkBuffer	indirectBuffer,
											 Bytes		indirectBufferOffset,
											 uint		drawCount,
											 Bytes		stride)
	{
		vkCmdDrawIndirect( _cmdbuf.Get(), indirectBuffer, VkDeviceSize(indirectBufferOffset), drawCount, CheckCast<uint>(stride) );
	}
	
/*
=================================================
	DrawIndexedIndirect
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::DrawIndexedIndirect (VkBuffer	indirectBuffer,
													Bytes		indirectBufferOffset,
													uint		drawCount,
													Bytes		stride)
	{
		vkCmdDrawIndexedIndirect( _cmdbuf.Get(), indirectBuffer, VkDeviceSize(indirectBufferOffset), drawCount, CheckCast<uint>(stride) );
	}
	
/*
=================================================
	DrawIndirectCount
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::DrawIndirectCount (VkBuffer	indirectBuffer,
												  Bytes		indirectBufferOffset,
												  VkBuffer	countBuffer,
												  Bytes		countBufferOffset,
												  uint		maxDrawCount,
												  Bytes		stride)
	{
		//CHECK_ERRV( GetFeatures().drawIndirectCount );

		vkCmdDrawIndirectCountKHR( _cmdbuf.Get(), indirectBuffer, VkDeviceSize(indirectBufferOffset), countBuffer, VkDeviceSize(countBufferOffset), maxDrawCount, CheckCast<uint>(stride) );
	}
	
/*
=================================================
	DrawIndexedIndirectCount
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::DrawIndexedIndirectCount (VkBuffer	indirectBuffer,
														 Bytes		indirectBufferOffset,
														 VkBuffer	countBuffer,
														 Bytes		countBufferOffset,
														 uint		maxDrawCount,
														 Bytes		stride)
	{
		//CHECK_ERRV( GetFeatures().drawIndirectCount );

		vkCmdDrawIndexedIndirectCountKHR( _cmdbuf.Get(), indirectBuffer, VkDeviceSize(indirectBufferOffset),
										  countBuffer, VkDeviceSize(countBufferOffset),
										  maxDrawCount, CheckCast<uint>(stride) );
	}
	
/*
=================================================
	_DrawMeshTasksNV
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::_DrawMeshTasksNV (uint taskCount, uint firstTask)
	{
	#ifdef VK_NV_mesh_shader
		//CHECK_ERRV( GetFeatures().meshShaderNV );
		
		vkCmdDrawMeshTasksNV( _cmdbuf.Get(), taskCount, firstTask );

	#else
		Unused( taskCount, firstTask );
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}
	
/*
=================================================
	DrawMeshTasksIndirectNV
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::DrawMeshTasksIndirectNV (VkBuffer	indirectBuffer,
														Bytes		indirectBufferOffset,
														uint		drawCount,
														Bytes		stride)
	{
	#ifdef VK_NV_mesh_shader
		//CHECK_ERRV( GetFeatures().meshShaderNV );
		
		vkCmdDrawMeshTasksIndirectNV( _cmdbuf.Get(), indirectBuffer, VkDeviceSize(indirectBufferOffset), drawCount, CheckCast<uint>(stride) );

	#else
		Unused( indirectBuffer, indirectBufferOffset, drawCount, stride );
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}
	
/*
=================================================
	DrawMeshTasksIndirectCountNV
=================================================
*/
	template <typename RM>
	void  _VDirectDrawCtx<RM>::DrawMeshTasksIndirectCountNV (VkBuffer	indirectBuffer,
															 Bytes		indirectBufferOffset,
															 VkBuffer	countBuffer,
															 Bytes		countBufferOffset,
															 uint		maxDrawCount,
															 Bytes		stride)
	{
	#ifdef VK_NV_mesh_shader
		//CHECK_ERRV( GetFeatures().meshShaderNV );
		
		vkCmdDrawMeshTasksIndirectCountNV( _cmdbuf.Get(), indirectBuffer, VkDeviceSize(indirectBufferOffset),
										   countBuffer, VkDeviceSize(countBufferOffset),
										   maxDrawCount, CheckCast<uint>(stride) );

	#else
		Unused( indirectBuffer, indirectBufferOffset, countBuffer, countBufferOffset, maxDrawCount, stride );
		AE_LOGE( "mesh shader is not supported!" );
	#endif
	}


}	// AE::Graphics::_hidden_

#endif	// AE_ENABLE_VULKAN
