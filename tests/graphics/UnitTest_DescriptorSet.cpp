// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "graphics/Public/DescriptorSet.h"
#include "graphics/Private/DescriptorSetHelper.h"
#include "stl/Containers/FixedTupleArray.h"
#include "UnitTest_Common.h"

namespace
{
	using Uniform			= DescriptorSetHelper::Uniform;
	using Uniforms_t		= DescriptorSetHelper::Uniforms_t;
	using EDescriptorType	= DescriptorSet::EDescriptorType;


	static void  DescriptorSet_Test1 ()
	{
		FixedTupleArray<8, UniformName, Uniform>	descriptors;

		{
			Uniform			un;
			un.type			= EDescriptorType::UniformBuffer;
			un.index		= 0;
			un.arraySize	= 1;
			un.buffer.dynamicOffsetIndex= 0;
			un.buffer.staticSize		= 128_b;
			descriptors.push_back( UniformName{"buf"}, un );
		}

		Uniforms_t		uniforms;
		uniforms.Get<0>() = uint(descriptors.size());
		uniforms.Get<1>() = descriptors.get<0>().data();
		uniforms.Get<2>() = descriptors.get<1>().data();
		
		DescriptorSet	ds;
		TEST( DescriptorSetHelper::Initialize( OUT ds, DescriptorSetLayoutID{0,0}, 0, DescriptorSetHelper::CreateDynamicData( uniforms, 1 )));

		TEST( ds.IsChanged() );
		TEST( ds.Index() == 0 );

		GfxResourceID	buf_id{ 0, 0, GfxResourceID::EType::Buffer };

		ds.BindBuffer( UniformName{"buf"}, buf_id, 64_b, 128_b );
		TEST( ds.GetDynamicOffsets().size() == 1 );
		TEST( ds.GetDynamicOffsets()[0] == 64_b );

		ds.SetBufferBase( UniformName{"buf"}, 32_b );
		TEST( ds.GetDynamicOffsets().size() == 1 );
		TEST( ds.GetDynamicOffsets()[0] == 32_b );
	}
}


extern void UnitTest_DescriptorSet ()
{
	DescriptorSet_Test1();

	AE_LOGI( "UnitTest_DescriptorSet - passed" );
}
