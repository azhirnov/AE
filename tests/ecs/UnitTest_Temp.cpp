// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ecs/Core/Registry.h"
#include "ecs/Core/MessageBuilder.h"
#include "UnitTest_Common.h"

/*
struct Comp1
{
	struct Tag_Changed {};
};

struct Comp2
{};

class System1 : public ECS::System
{
	void operator () (ReadAccess<EntityID> ids, WriteAccess<Comp1> a, Subtractive<Comp2>, MessageBuilder &msg)
	{
		msg.Add<Comp1::Tag_Changed>(id, a);
	}
};*/


extern void UnitTest_Temp ()
{



	AE_LOGI( "UnitTest_Temp - passed" );
}
