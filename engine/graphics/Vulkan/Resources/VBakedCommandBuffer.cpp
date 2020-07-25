// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#ifdef AE_ENABLE_VULKAN

# include "graphics/Vulkan/Resources/VBakedCommandBuffer.h"
# include "graphics/Vulkan/VResourceManager.h"

namespace AE::Graphics
{

/*
=================================================
	destructor
=================================================
*/
	VBakedCommandBuffer::~VBakedCommandBuffer ()
	{
		CHECK( not _cmdBuf );
		CHECK( not _cmdPool );
	}
	
/*
=================================================
	Create
=================================================
*/
	bool  VBakedCommandBuffer::Create (const VDevice &dev, const VCommandPoolPtr &pool, RenderPassID rp)
	{
		EXLOCK( _drCheck );
		CHECK_ERR( pool );
		CHECK_ERR( not _cmdBuf );
			
		_cmdBuf = pool->Alloc( dev );
		CHECK_ERR( _cmdBuf );

		_cmdPool		= pool;
		_renderPassId	= rp;

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void  VBakedCommandBuffer::Destroy (VResourceManager &resMngr)
	{
		EXLOCK( _drCheck );

		if ( _cmdPool and _cmdBuf )
			_cmdPool->Recycle( resMngr.GetDevice(), _cmdBuf );
		
		_resources.Release( resMngr );

		_cmdPool		= null;
		_cmdBuf			= VK_NULL_HANDLE;
		_renderPassId	= Default;

		_barriers.clear();
	}
	

}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
