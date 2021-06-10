// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Limitations:
		- single-threaded
*/

#include "stl/Math/Math.h"
#include "stl/Math/Vec.h"
#include "stl/Algorithms/StringUtils.h"

using namespace AE::STL;
using namespace AE::Math;

namespace
{
	enum class ExeOrder : uint {};
	
	struct RenderTarget;


	struct RTUsage
	{
		ExeOrder	exeOrder	= ExeOrder(~0u);
		String		access;

		RTUsage (ExeOrder order, StringView access) : exeOrder{order}, access{access} {}
	};


	struct RTMemoryBlock
	{
		uint		index	= ~0u;
		uint		size	= 0;
		ExeOrder	min		= ExeOrder(~0u);
		ExeOrder	max		= ExeOrder(0);

		HashSet< RenderTarget *>	rts;
	};


	struct RenderTarget
	{
		friend class RTManager;
	public:
		uint2	dim;
		uint	samples	= 1;
		String	name;

		RenderTarget () {}

		ExeOrder  StartPoint ()	const	{ return _usage.size() ? _usage.front().exeOrder : ExeOrder(0); }
		ExeOrder  EndPoint ()	const	{ return _usage.size() ? _usage.back().exeOrder : ExeOrder(~0u); }

	private:
		uint				_size	= 0;
		Array< RTUsage >	_usage;
		RTMemoryBlock *		_mem	= null;
	};



	class RTManager
	{
	private:
		Deque< RenderTarget >			_rtPool;
		Array< Array< RenderTarget *>>	_rtUsage;

	public:
		RTManager () {}

		RenderTarget*  CreateRT (uint2 dim, uint ms, StringView name);

		void  AddUsage (RenderTarget* rt, ExeOrder order, StringView access);

		String  Build ();
	};
	

	RenderTarget*  RTManager::CreateRT (uint2 dim, uint ms, StringView name)
	{
		auto&	rt = _rtPool.emplace_back();
		rt.dim		= Max( 1u, dim );
		rt.samples	= Max( 1u, ms );
		rt.name		= name;
		rt._size	= rt.dim.x * rt.dim.y * rt.samples;
		rt._mem		= null;
		return &rt;
	}
	

	void  RTManager::AddUsage (RenderTarget* rt, ExeOrder order, StringView access)
	{
		rt->_usage.push_back({ order, access });

		_rtUsage.resize( uint(order) + 1 );
		_rtUsage[ uint(order) ].push_back( rt );
	}
	

	String  RTManager::Build ()
	{
		Deque<RTMemoryBlock>	mem_blocks;

		const auto	AddRT = [&mem_blocks] (RenderTarget* rt, uint exeOrder)
		{
			ASSERT( rt->_mem == null );

			RTMemoryBlock*	best_match	= null;
			float			mem_size_f	= 0.0f;

			for (auto& mem : mem_blocks)
			{
				if ( not mem.rts.empty() )
					continue;
				
				if ( not best_match )
				{
					best_match = &mem;
					continue;
				}

				float	f = float(mem.size) / float(rt->_size);

				if ( best_match and Abs(1.0f - f) < Abs(1.0f - mem_size_f) )
				{
					best_match = &mem;
					mem_size_f = f;
				}
			}

			if ( best_match == null )
			{
				auto&	mem = mem_blocks.emplace_back();
				mem.index	= uint(mem_blocks.size()-1);
				best_match = &mem;
			}

			{
				auto&	mem = *best_match;
				mem.rts.insert( rt );
				rt->_mem = &mem;

				mem.size	= Max( mem.size, rt->_size );
				mem.min		= ExeOrder( Min( uint(mem.min), exeOrder ));
				mem.max		= ExeOrder( Max( uint(mem.max), exeOrder ));
			}
		};

		const auto	RemoveRT = [] (RenderTarget* rt, uint exeOrder)
		{
			ASSERT( rt->_mem );

			auto&	mem = *rt->_mem;
			mem.min = ExeOrder( Min( uint(mem.min), exeOrder ));
			mem.max = ExeOrder( Max( uint(mem.max), exeOrder ));
			mem.rts.erase( rt );
			//rt->_mem = null;
		};

		String	str;
		ulong	unopt_mem_size	= 0;
		ulong	opt_mem_size	= 0;
		ulong	max_mem_usage	= 0;
		ulong	mem_usage		= 0;

		//for (usize i = _rtUsage.size()-1; i < _rtUsage.size(); --i)
		for (usize i = 0; i < _rtUsage.size(); ++i)
		{
			for (auto* rt : _rtUsage[i])
			{
				// first usage
				if ( rt->StartPoint() == ExeOrder(i) )
				{
					AddRT( rt, uint(i) );
					mem_usage += rt->_size;
				}
				else
				// last usage
				if ( rt->EndPoint() == ExeOrder(i) )
				{
					RemoveRT( rt, uint(i) );
					mem_usage -= rt->_size;
				}
			}
			max_mem_usage = Max( mem_usage, max_mem_usage );
		}

		for (auto& rt : _rtPool)
		{
			unopt_mem_size += rt._size;
		}
		for (auto& mem : mem_blocks)
		{
			ASSERT( mem.rts.empty() );
			opt_mem_size += mem.size;
		}

		str << "\nOrigin memory size:    " << ToString( Bytes{unopt_mem_size} );
		str << "\nOptimized memory size: " << ToString( Bytes{opt_mem_size} );
		str << "\nOptimization:          " << ToString( (float(unopt_mem_size) - float(opt_mem_size)) / unopt_mem_size * 100.0f, 1 ) << " %";
		str << "\nMemory overhead:       " << ToString( (float(opt_mem_size) - float(max_mem_usage)) / max_mem_usage * 100.0f, 1 ) << " %";
		
		for (auto& rt : _rtPool)
		{
			ASSERT( rt._mem );
			str << "\n    rt: '" << rt.name << "', mem: " << ToString( rt._mem->index ) << ", used: " << ToString( float(rt._size) / rt._mem->size * 100.0f, 1 ) << " %";
		}
		for (auto& mem : mem_blocks)
		{
			str << "\n    mem: " << ToString( mem.index ) << ", range: [" << ToString( uint(mem.min) ) << ", " << ToString( uint(mem.max) ) << "], size: " << ToString( Bytes{mem.size} );
		}
		
		str << "\n----------";

		return str;
	}


	static void RTMM_Test1 ()
	{
		RTManager	rt_mngr;
		uint		cmd		= 0;

		auto*	rt1	= rt_mngr.CreateRT( uint2{1024, 1024}, 1, "RT-1" );
		auto*	ct	= rt_mngr.CreateRT( uint2{1920, 1080}, 4, "Color Target" );
		auto*	dst	= rt_mngr.CreateRT( uint2{1920, 1080}, 4, "DepthStencil Target" );
		auto*	rt2	= rt_mngr.CreateRT( uint2{1920, 1080}, 1, "RT-2" );

		// pass 1
		rt_mngr.AddUsage( rt1, ExeOrder(cmd), "color_attachment" );
		++cmd;

		// pass 2 (g-buffer)
		rt_mngr.AddUsage( ct,  ExeOrder(cmd), "color_attachment" );
		rt_mngr.AddUsage( dst, ExeOrder(cmd), "depth_attachment" );
		rt_mngr.AddUsage( rt1, ExeOrder(cmd), "sample" );
		++cmd;
	
		// pass 3 (post-process)
		rt_mngr.AddUsage( rt2, ExeOrder(cmd), "color_attachment" );
		rt_mngr.AddUsage( ct,  ExeOrder(cmd), "sample" );
		rt_mngr.AddUsage( dst, ExeOrder(cmd), "sample" );
		++cmd;
		
		// pass 4 (present)
		rt_mngr.AddUsage( rt2, ExeOrder(cmd), "read" );


		String	res = rt_mngr.Build();
		AE_LOGI( res );
	}

	
	static void RTMM_Test2 ()
	{
		RTManager	rt_mngr;
		uint		cmd		= 0;

		auto*	ao		= rt_mngr.CreateRT( uint2{1920, 1080} / 2,	1, "SSAO" );
		auto*	g_col	= rt_mngr.CreateRT( uint2{1920, 1080},		1, "G-Buffer Color" );
		auto*	g_dep	= rt_mngr.CreateRT( uint2{1920, 1080},		1, "G-Buffer Depth" );
		auto*	g_nor	= rt_mngr.CreateRT( uint2{1920, 1080},		1, "G-Buffer Norm" );
		auto*	g_hdr	= rt_mngr.CreateRT( uint2{1920, 1080},		2, "G-Buffer HDR light" );
		auto*	parts	= rt_mngr.CreateRT( uint2{1024, 1024},		1, "Particles" );
		auto*	rt1		= rt_mngr.CreateRT( uint2{1920, 1080},		2, "RT-1" );
		auto*	rt2		= rt_mngr.CreateRT( uint2{1920, 1080},		2, "RT-2" );
		auto*	rt3		= rt_mngr.CreateRT( uint2{1920, 1080},		2, "RT-3" );

		// pass 1 (depth prepass)
		rt_mngr.AddUsage( g_dep, ExeOrder(cmd), "depth_attachment" );
		++cmd;
	
		// pass 2 (AO)
		rt_mngr.AddUsage( ao,	 ExeOrder(cmd), "color_attachment" );
		rt_mngr.AddUsage( g_dep, ExeOrder(cmd), "depth_read" );
		++cmd;

		// pass 3 (g-buffer)
		rt_mngr.AddUsage( g_col, ExeOrder(cmd), "color_attachment" );
		rt_mngr.AddUsage( g_nor, ExeOrder(cmd), "color_attachment" );
		rt_mngr.AddUsage( g_hdr, ExeOrder(cmd), "color_attachment" );
		rt_mngr.AddUsage( g_dep, ExeOrder(cmd), "depth_read" );
		rt_mngr.AddUsage( ao,	 ExeOrder(cmd), "sample" );
		++cmd;
		
		// pass 4 (lighting)
		rt_mngr.AddUsage( rt1,	 ExeOrder(cmd), "color_attachment" );
		rt_mngr.AddUsage( g_col, ExeOrder(cmd), "sample" );
		rt_mngr.AddUsage( g_hdr, ExeOrder(cmd), "sample" );
		rt_mngr.AddUsage( g_dep, ExeOrder(cmd), "depth_read" );
		++cmd;

		// pass 5 (particles)
		rt_mngr.AddUsage( parts, ExeOrder(cmd), "color_attachment" );
		rt_mngr.AddUsage( g_dep, ExeOrder(cmd), "depth_read" );
		++cmd;
		
		// pass 6 (translucent)
		rt_mngr.AddUsage( rt2,	 ExeOrder(cmd), "color_attachment" );
		rt_mngr.AddUsage( parts, ExeOrder(cmd), "sample" );
		rt_mngr.AddUsage( rt1,	 ExeOrder(cmd), "sample" );
		++cmd;

		// pass 7 (post-process)
		rt_mngr.AddUsage( rt3,	 ExeOrder(cmd), "color_attachment" );
		rt_mngr.AddUsage( rt2,	 ExeOrder(cmd), "sample" );
		rt_mngr.AddUsage( g_nor, ExeOrder(cmd), "sample" );
		++cmd;
		
		// pass 8 (present)
		rt_mngr.AddUsage( rt3,	 ExeOrder(cmd), "read" );

		
		String	res = rt_mngr.Build();
		AE_LOGI( res );
	}
}


extern void Alg_RTMemoryManager ()
{
	AE_LOGI( "Render target memory manager" );
	RTMM_Test1();
	RTMM_Test2();
	AE_LOGI( "=============================\n\n" );
}
