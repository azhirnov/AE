// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "stl/Stream/MemStream.h"
#include "UnitTest_Common.h"

namespace
{
	struct SerObj
	{
		int		i;
		float	f;

		SerObj () {}
	};

	bool SerObj_Serialize (Serializer &ser, const void *ptr)
	{
		auto*	self = Cast<SerObj>(ptr);

		ser( self->i, self->f );
		return true;
	}

	bool SerObj_Deserialize (const Deserializer &deser, OUT void *ptr, bool create)
	{
		auto*	self = create ? PlacementNew<SerObj>(ptr) : Cast<SerObj>(ptr);
	
		deser( self->i, self->f );
		return true;
	}


	void Serialization_Test1 ()
	{
		ObjectFactory	factory;
		TEST( factory.Register<SerObj>( SerializedID{"Test1"}, SerObj_Serialize, SerObj_Deserialize ));

		SerObj	a, b, c;
		auto	stream = MakeShared<MemWStream>();

		a.i = 1111;
		a.f = 2.5463434f;

		Serializer		ser;
		ser.stream		= stream;
		ser.factory		= &factory;
		TEST( ser( a ));

		Deserializer	deser;
		deser.stream	= MakeShared<MemRStream>( stream->GetData() );
		deser.factory	= &factory;
		TEST( deser( b ));
	
		deser.stream	= MakeShared<MemRStream>( stream->GetData() );
		TEST( deser( static_cast<void *>(&c) ));

		TEST( a.i == b.i );
		TEST( a.f == b.f );
		TEST( a.i == c.i );
		TEST( a.f == c.f );
	}
}


extern void UnitTest_Serialization ()
{
	Serialization_Test1();

	AE_LOGI( "UnitTest_Serialization - passed" );
}
