// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "CPP_VM/VirtualMachine.h"
#include "CPP_VM/Atomic.h"
#include "CPP_VM/Storage.h"
#include "UnitTest_Common.h"

using namespace LFAS;
using namespace LFAS::CPP;

#include "threading/Primitives/SpinLock.h"

namespace
{
	using AE::Threading::SpinLock;


	void SpinLock_Test1 ()
	{
		VirtualMachine::CreateInstance();
		{
			struct Data
			{
				int		i;
				float	f;
				double	d;
			};

			struct
			{
				SpinLock		guard;
				Storage<Data>	data;

			}	global;
		
			auto&	vm = VirtualMachine::Instance();
			vm.ThreadFenceRelease();

			auto	sc1 = vm.CreateScript( [g = &global, fcnt = 0.0f] () mutable
							{
								EXLOCK( g->guard );
								Unused( g->data.Read( &Data::i ));
								g->data.Write( &Data::f, fcnt += 1.0f );
							});
	
			auto	sc2 = vm.CreateScript( [g = &global, icnt = 0] () mutable
							{
								EXLOCK( g->guard );
								g->data.Write( &Data::i, ++icnt );
								g->data.Write( &Data::d, double(g->data.Read( &Data::f )) );
							});

			vm.RunParallel({ sc1, sc2 }, SecondsF{10.0f} );
		
			vm.ThreadFenceAcquire();
		}
		VirtualMachine::DestroyInstance();
	}
}


extern void Test_SpinLock ()
{
	SpinLock_Test1();

	AE_LOGI( "Test_SpinLock - passed" );
}
