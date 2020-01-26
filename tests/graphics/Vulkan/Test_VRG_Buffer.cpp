// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "Test_VulkanRenderGraph.h"

bool VRGTest::Test_Buffer ()
{
	BufferDesc	desc;
	desc.size	= 32_b;
	desc.usage	= EBufferUsage::Uniform;

	auto id = _resourceMngr->CreateBuffer( desc );
	
	_resourceMngr->ReleaseResource( id );

	AE_LOGI( TEST_NAME << " - passed" );
	return true;
}
