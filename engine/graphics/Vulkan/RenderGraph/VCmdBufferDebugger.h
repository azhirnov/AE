// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#ifdef AE_ENABLE_VULKAN
# include "graphics/Public/CommandBuffer.h"
# include "graphics/Vulkan/VRenderGraph.h"

namespace AE::Graphics
{

	//
	// Vulkan Command Buffer Debugger
	//

	template <typename BaseCmdBuf>
	class VCmdBufferDebugger : public BaseCmdBuf
	{
	// variables
	private:
		Array<String>		_markers;
		UniqueID<BufferID>	_dbgBuffer;			// buffer with host visible memory
		Bytes				_dbgBufferSize;
		Bytes				_dbgBufferOffset;


	// methods
	public:
		explicit VCmdBufferDebugger (const VCommandBatch& batch) : BaseCmdBuf{batch}
		{
			_markers.reserve( 100 );
		}

		void  AddMarker (StringView text)
		{
			if ( _dbgBufferOffset > _dbgBufferSize )
				return;

			const uint	id = uint(_markers.size());
			_markers.emplace_back( text );

			FillBuffer( _dbgBuffer, _dbgBufferOffset, SizeOf<uint>, id );
			_dbgBufferOffset += SizeOf<uint>;
		}
	};


}	// AE::Graphics

#endif	// AE_ENABLE_VULKAN
