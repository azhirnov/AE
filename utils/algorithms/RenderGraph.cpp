// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	TODO:
		- resource transitions
			- check for parallel writing
			- check that exclusive resource have not readn from multiple queues
		- without execution dependencies
*/

#include "stl/Containers/ArrayView.h"
#include "stl/Algorithms/StringUtils.h"

using namespace AE::STL;
using namespace AE::Math;

namespace V1
{
	enum class EQueue
	{
		Graphics,
		AsyncCompute,
		AsyncTransfer,
		_Count,
	};

	static String  QueueToString (EQueue q)
	{
		switch ( q )
		{
			case EQueue::Graphics :			return "Graphics";
			case EQueue::AsyncCompute :		return "AsyncCompute";
			case EQueue::AsyncTransfer :	return "AsyncTransfer";
		}
		return "<unknown>";
	}

	enum class TaskId : uint {};

	enum class ResId : uint {};


	struct Resource
	{
		ResId	id;
		String	usage;

		Resource (ResId id, StringView usage) : id{id}, usage{usage} {}
	};


	class RenderGraph
	{
	private:
		struct Task
		{
			EQueue			queue		= EQueue::_Count;
			String			name;
			Array<Resource>	resources;
			Array<TaskId>	deps;

			usize			index		= UMax;

			ND_ bool  IsProcessed () const	{ return index > 0; }
		};


	private:
		uint			_taskCounter	= 0;
		Array<Task>		_tasks;


	public:
		RenderGraph () {}

		TaskId  AddTask (EQueue queue, StringView name, ArrayView<Resource> mutableResources = {}, ArrayView<TaskId> deps = {});

		String  Build ();
	};

	

	TaskId  RenderGraph::AddTask (EQueue queue, StringView name, ArrayView<Resource> mutableResources, ArrayView<TaskId> deps)
	{
		_tasks.resize( _taskCounter + 1 );

		auto&	task	= _tasks[ _taskCounter ];
		task.queue		= queue;
		task.name		= name;
		task.index		= 0;	// invalid index
		task.resources.assign( mutableResources.begin(), mutableResources.end() );
		task.deps.assign( deps.begin(), deps.end() );

		return TaskId(_taskCounter++);
	}


	String  RenderGraph::Build ()
	{
		struct QueueData
		{
			Array<Task *>								tasks;
			StaticArray< usize, uint(EQueue::_Count) >	lastVisId;	// task index that synchronized with another queue

			QueueData ()
			{
				lastVisId.fill( 0 );
			}
		};

		using PerQueue	= StaticArray< QueueData, uint(EQueue::_Count) >;
		using QBits		= BitSet< uint(EQueue::_Count) >;

		String		str;
		PerQueue	per_queue;
		Deque<Task>	sync_tasks;
		usize		task_idx	= 0;

		for (;;)
		{
			usize	cnt = 0;

			for (auto& task : _tasks)
			{
				if ( task.IsProcessed() )
				{
					++cnt;
					continue;
				}

				bool	complete = true;
				QBits	q_mask {0};

				for (auto& dep : task.deps)
				{
					auto&	dep_t = _tasks[uint(dep)];
					complete &= dep_t.IsProcessed();

					if ( dep_t.queue != task.queue )
					{
						if ( dep_t.index > per_queue[uint(task.queue)].lastVisId[uint(dep_t.queue)] )
							q_mask.set( uint(dep_t.queue) );
					}
				}

				if ( not complete )
					continue;
				
				ASSERT( not q_mask[uint(task.queue)] );

				if ( q_mask.to_ulong() )
				{
					++task_idx;
					for (uint q = 0; q < q_mask.size(); ++q)
					{
						if ( q_mask[q] )
						{
							auto&	sync = sync_tasks.emplace_back();
							sync.name = "["s << ToString(task_idx) << "] semaphore ("s << QueueToString( EQueue(q) ) << " --> " << QueueToString( task.queue ) << ')';

							per_queue[q].tasks.push_back( &sync );
							per_queue[uint(task.queue)].tasks.push_back( &sync );
							per_queue[uint(task.queue)].lastVisId[q] = task_idx;
						}
					}
				}
				
				++task_idx;

				task.index = task_idx;
				task.name = "["s << ToString(task_idx) << "] " << task.name;

				per_queue[uint(task.queue)].tasks.push_back( &task );
			}

			if ( cnt == _tasks.size() )
				break;
		}

		for (usize i = 0; i < per_queue.size(); ++i)
		{
			if ( per_queue[i].tasks.empty() )
				continue;

			str << '\n' << QueueToString( EQueue(i) );

			for (auto& task : per_queue[i].tasks)
			{
				str << "\n    " << task->name;
			}

			str << "\n----------";
		}

		return str;
	}


	static void RG_Test1 ()
	{
		RenderGraph		rg;
		const auto		g_dep	= ResId(1);		// g-buffer depth
		const auto		g_col	= ResId(2);		// g-buffer color
		const auto		g_nor	= ResId(3);		// g-buffer normals
		const auto		i_shad	= ResId(4);		// shadow image
		const auto		b_parts	= ResId(5);		// buffer with particles
		const auto		i_ssao	= ResId(6);		// screen-space ambient occlusion
		const auto		i_ssr	= ResId(7);		// screen-space reflections
		const auto		i_scr	= ResId(8);		// static cubemap reflections
		const auto		i_refl	= ResId(9);		// combined reflections
		const auto		i_emis	= ResId(10);	// emissive objects
		const auto		i_ui	= ResId(11);	// user interface
		
		TaskId	upload	= rg.AddTask( EQueue::AsyncTransfer,
									  "Upload resources",
									  {},
									  {} ); // at the end of this pass transit resources to read-only state

		TaskId	update	= rg.AddTask( EQueue::Graphics,
									  "Update resources",
									  {},
									  {} ); // at the end of this pass transit resources to read-only state

		TaskId	shadow	= rg.AddTask( EQueue::Graphics,
									  "Shadow pass",
									  {{i_shad, "depth target"}},
									  {update, upload} );

		TaskId	dpp		= rg.AddTask( EQueue::Graphics,
									  "Depth pre-pass",
									  {{g_dep, "depth target"}},
									  {update, upload} );

		TaskId	ssao	= rg.AddTask( EQueue::AsyncCompute,
									  "SSAO",
									  {{g_dep, "read depth"},
									   {i_ssao, "write"}},
									  {dpp} );

		TaskId	gbuf	= rg.AddTask( EQueue::Graphics,
									  "G-Buffer pass",
									  {{g_dep, "read depth"},
									   {g_col, "color attachment"},
									   {g_nor, "color attachment"},
									   {i_shad, "sample"}},
									  {dpp, shadow} );

		TaskId	parts	= rg.AddTask( EQueue::AsyncCompute,
									  "Sim particles",
									  {{b_parts, "update buffer"},
									   {g_dep, "read depth"}},
									  {dpp} );
		
		TaskId	ssr		= rg.AddTask( EQueue::Graphics,
									  "SSR",
									  {{g_dep, "read depth"},
									   {g_col, "sample"},
									   {i_ssr, "color attachment"}},
									  {gbuf} );

		TaskId	scr		= rg.AddTask( EQueue::Graphics,
									  "SCR",
									  {{g_dep, "read depth"},
									   {i_scr, "color attachment"}},
									  {gbuf} );
		
		TaskId	com_ref = rg.AddTask( EQueue::Graphics,
									  "Combine reflections",
									  {{i_ssao, "sample"},
									   {i_ssr, "sample"},
									   {i_scr, "sample"},
									   {i_refl, "color attachment"}},
									  {ssr, scr, ssao} );
		
		TaskId	emis	= rg.AddTask( EQueue::Graphics,
									  "Emissive pass",
									  {{g_dep, "read depth"},
									   {i_emis, "color attachment"}},
									  {gbuf, parts} );
		
		TaskId	ui		= rg.AddTask( EQueue::Graphics,
									  "UI",
									  {{i_ui, "color attachment"}},
									  {} );
		
		TaskId	fin		= rg.AddTask( EQueue::Graphics,
									  "Final",
									  {{i_ui, "sample"},
									   {i_emis, "sample"},
									   {i_refl, "sample"}},
									  {ui, emis, com_ref} );
		Unused( fin );

		String	res = rg.Build();
		AE_LOGI( res );
	}
}


extern void Alg_RenderGraph ()
{
	AE_LOGI( "Render graph" );
	V1::RG_Test1();
	AE_LOGI( "=============================\n\n" );
}
