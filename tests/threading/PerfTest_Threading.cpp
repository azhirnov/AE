// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#include "threading/TaskSystem/TaskScheduler.h"
#include "threading/TaskSystem/WorkerThread.h"
#include "threading/TaskSystem/FunctionTask.h"

#include "stl/Math/Vec.h"
#include "stl/Algorithms/StringUtils.h"
#include "PerlinNoise.hpp"

#include "UnitTest_Common.h"

using namespace AE::Threading;

namespace
{
	static const siv::PerlinNoise	noise;
	static const uint				max_levels			= 6;
	static const uint				max_levels2			= max_levels * max_levels;
	static const uint				octave_count		= 4;
	static Atomic<ulong>			task_complete		{0};
	static Atomic<ulong>			task_payload_time	{0};
	static Atomic<ulong>			task_counter		{0};
	
	using TimePoint_t	= std::chrono::high_resolution_clock::time_point;
	using EThread		= IAsyncTask::EThread;


	class HeightMap : public EnableRC<HeightMap>
	{
	public:
		static constexpr uint			W = 4;
		static constexpr uint			H = 4;
		StaticArray< ubyte, W*H >		data;

		HeightMap () {}
	};
	using HeightMap_t = RC< HeightMap >;


	class FinalTask final : public IAsyncTask
	{
	public:
		FinalTask () : IAsyncTask{EThread::Worker} {}

		void Run () override
		{
			task_complete.fetch_add( 1, EMemoryOrder::Relaxed );
		}
	};
}


namespace
{
	class LargeTask1 final : public IAsyncTask
	{
	private:
		const uint2		_cell;
		const uint		_level;
		HeightMap_t		_result;

	public:
		LargeTask1 (const uint2 &cell, uint level) :
			IAsyncTask{ EThread::Worker },
			_cell{ cell }, _level{ level }, _result{ MakeRC<HeightMap>() }
		{}

	private:
		void Run () override
		{
			const TimePoint_t	start	= TimePoint_t::clock::now();
			const double2		size	{ Pow( 0.5, double(_level) )};
			const double2		offset	= double2(_cell) * size;

			for (uint y = 0; y < HeightMap::H; ++y)
			for (uint x = 0; x < HeightMap::W; ++x)
			{
				double2	c = offset + (double2(x, y) / double2(HeightMap::W, HeightMap::H)) * size;
				_result->data[x + y * HeightMap::W] = ubyte( noise.octaveNoise( c.x, c.y, octave_count ) * 255.0 );
			}

			task_payload_time.fetch_add( (TimePoint_t::clock::now() - start).count(), EMemoryOrder::Relaxed );
			task_counter.fetch_add( 1, EMemoryOrder::Relaxed );

			if ( _level < max_levels )
			{
				Scheduler().Run<LargeTask1>( Tuple{_cell * 2 + uint2(0,0), _level+1} );
				Scheduler().Run<LargeTask1>( Tuple{_cell * 2 + uint2(0,1), _level+1} );
				Scheduler().Run<LargeTask1>( Tuple{_cell * 2 + uint2(1,0), _level+1} );
				Scheduler().Run<LargeTask1>( Tuple{_cell * 2 + uint2(1,1), _level+1} );
			}
			else
			{
				Scheduler().Run<FinalTask>();
			}
		}

		void OnCancel () override
		{
			TEST(false);
		}
	};

	static void  Threading_Test1 ()
	{
		task_complete.store( 0 );
		task_payload_time.store( 0 );
		task_counter.store( 0 );

		const usize			num_threads = std::thread::hardware_concurrency()-1;
		LocalTaskScheduler	scheduler	{num_threads};
		{
			for (usize i = 0; i < num_threads; ++i) {
				scheduler->AddThread( MakeRC<WorkerThread>(
					WorkerThread::ThreadMask{}.set(uint(WorkerThread::EThread::Worker)),
					nanoseconds{i > 0 ? 1 : 0},
					nanoseconds{i > 0 ? 10 : 0} // only one worker thread should never sleep
				));
			}

			const auto	start_time = TimePoint_t::clock::now();
		
			scheduler->Run<LargeTask1>( Tuple{uint2{0,0}, 0u} );
			scheduler->Run<LargeTask1>( Tuple{uint2{0,1}, 0u} );
			scheduler->Run<LargeTask1>( Tuple{uint2{1,0}, 0u} );
			scheduler->Run<LargeTask1>( Tuple{uint2{1,1}, 0u} );

			const ulong	required = 1ull << (2 + 2 * max_levels);

			for (;;)
			{
				if ( task_complete.load( EMemoryOrder::Relaxed ) >= required )
					break;

				scheduler->ProcessTask( EThread::Worker, 0 );
			}
			
			const nanoseconds	total_time	= TimePoint_t::clock::now() - start_time;
			const nanoseconds	total_time2	= total_time * (num_threads + 1);
			const double		overhead	= double(total_time2.count() - task_payload_time.load()) / double(total_time2.count());
			const ulong			task_count	= task_counter.load();

			AE_LOGI( "Total time: "s << ToString( total_time ) <<
					 ", final jobs: " << ToString( task_counter ) <<
					 ", task time: " << ToString( nanoseconds{ task_payload_time.load() / task_counter }) <<
					 ", overhead: " << ToString( overhead * 100.0, 2 ) << " %" );
		}
	}
}


namespace
{
	class LargeTask2 final : public IAsyncTask
	{
	private:
		const uint2		_cell;
		const uint		_level;
		HeightMap_t		_result;

	public:
		LargeTask2 (const uint2 &cell, uint level) :
			IAsyncTask{ EThread::Worker },
			_cell{ cell }, _level{ level }, _result{ MakeRC<HeightMap>() }
		{}

	private:
		void Run () override
		{
			const TimePoint_t	start	= TimePoint_t::clock::now();
			const double2		size	{ Pow( 0.5, double(_level) )};
			const double2		offset	= double2(_cell) * size;

			for (uint y = 0; y < HeightMap::H; ++y)
			for (uint x = 0; x < HeightMap::W; ++x)
			{
				double2	c = offset + (double2(x, y) / double2(HeightMap::W, HeightMap::H)) * size;
				_result->data[x + y * HeightMap::W] = ubyte( noise.octaveNoise( c.x, c.y, octave_count ) * 255.0 );
			}
			
			task_payload_time.fetch_add( (TimePoint_t::clock::now() - start).count(), EMemoryOrder::Relaxed );
			task_counter.fetch_add( 1, EMemoryOrder::Relaxed );

			if ( _level < max_levels )
			{
				auto	t0 = Scheduler().Run<LargeTask2>( Tuple{_cell * 2 + uint2(0,0), _level+1} );
				auto	t1 = Scheduler().Run<LargeTask2>( Tuple{_cell * 2 + uint2(0,1), _level+1}, Tuple{t0} );
				auto	t2 = Scheduler().Run<LargeTask2>( Tuple{_cell * 2 + uint2(1,0), _level+1}, Tuple{t0, t1} );
				auto	t3 = Scheduler().Run<LargeTask2>( Tuple{_cell * 2 + uint2(1,1), _level+1}, Tuple{t0, t1, t2} );
				Unused( t3 );
			}
			else
			{
				Scheduler().Run<FinalTask>();
			}
		}

		void OnCancel () override
		{
			TEST(false);
		}
	};

	static void  Threading_Test2 ()
	{
		task_complete.store( 0 );
		task_payload_time.store( 0 );
		task_counter.store( 0 );
		
		const usize			num_threads = std::thread::hardware_concurrency()-1;
		LocalTaskScheduler	scheduler	{num_threads};
		{
			for (usize i = 0; i < num_threads; ++i) {
				scheduler->AddThread( MakeRC<WorkerThread>(
					WorkerThread::ThreadMask{}.set(uint(WorkerThread::EThread::Worker)),
					nanoseconds{i > 0 ? 1 : 0},
					nanoseconds{i > 0 ? 10 : 0} // only one worker thread should never sleep
				));
			}

			const auto	start_time = TimePoint_t::clock::now();
		
			scheduler->Run<LargeTask2>( Tuple{uint2{0,0}, 0u} );
			scheduler->Run<LargeTask2>( Tuple{uint2{0,1}, 0u} );
			scheduler->Run<LargeTask2>( Tuple{uint2{1,0}, 0u} );
			scheduler->Run<LargeTask2>( Tuple{uint2{1,1}, 0u} );
						
			const ulong	required = 1ull << (2 + 2 * max_levels);
			
			for (;;)
			{
				if ( task_complete.load( EMemoryOrder::Relaxed ) >= required )
					break;

				scheduler->ProcessTask( EThread::Worker, 0 );
			}
			
			const nanoseconds	total_time	= TimePoint_t::clock::now() - start_time;
			const nanoseconds	total_time2	= total_time * (num_threads + 1);
			const double		overhead	= double(total_time2.count() - task_payload_time.load()) / double(total_time2.count());
			const ulong			task_count	= task_counter.load();

			AE_LOGI( "Total time: "s << ToString( total_time ) <<
					 ", final jobs: " << ToString( task_counter ) <<
					 ", task time: " << ToString( nanoseconds{ task_payload_time.load() / task_counter }) <<
					 ", overhead: " << ToString( overhead * 100.0, 2 ) << " %" );
		}
	}
}


namespace
{
	class LargeTask3 final : public IAsyncTask
	{
	private:
		const uint2		_cell;
		const uint		_level;
		HeightMap_t		_result;

	public:
		LargeTask3 (const uint2 &cell, uint level) :
			IAsyncTask{ EThread::Worker },
			_cell{ cell }, _level{ level }, _result{ MakeRC<HeightMap>() }
		{}

	private:
		void Run () override
		{
			if ( _level < max_levels2 )
			{
				Scheduler().Run<LargeTask3>( Tuple{_cell * 2 + uint2(0,0), _level+1} );
			}

			const TimePoint_t	start	= TimePoint_t::clock::now();
			const double2		size	{ Pow( 0.5, double(_level) )};
			const double2		offset	= double2(_cell) * size;

			for (uint y = 0; y < HeightMap::H; ++y)
			for (uint x = 0; x < HeightMap::W; ++x)
			{
				double2	c = offset + (double2(x, y) / double2(HeightMap::W, HeightMap::H)) * size;
				_result->data[x + y * HeightMap::W] = ubyte( noise.octaveNoise( c.x, c.y, octave_count ) * 255.0 );
			}
			
			task_payload_time.fetch_add( (TimePoint_t::clock::now() - start).count(), EMemoryOrder::Relaxed );
			task_counter.fetch_add( 1, EMemoryOrder::Relaxed );
			
			if ( not (_level < max_levels2) )
			{
				Scheduler().Run<FinalTask>();
			}
		}

		void OnCancel () override
		{
			TEST(false);
		}
	};

	static void  Threading_Test3 ()
	{
		task_complete.store( 0 );
		task_payload_time.store( 0 );
		task_counter.store( 0 );
		
		const usize			num_threads = std::thread::hardware_concurrency()-1;
		LocalTaskScheduler	scheduler	{num_threads};
		{
			for (usize i = 0; i < num_threads; ++i) {
				scheduler->AddThread( MakeRC<WorkerThread>(
					WorkerThread::ThreadMask{}.set(uint(WorkerThread::EThread::Worker)),
					nanoseconds{i > 0 ? 1 : 0},
					nanoseconds{i > 0 ? 10 : 0} // only one worker thread should never sleep
				));
			}

			const auto	start_time	= TimePoint_t::clock::now();
			const uint	grid_size	= std::thread::hardware_concurrency() * 16;
		
			for (uint y = 0; y < grid_size; ++y)
			for (uint x = 0; x < grid_size; ++x)
				scheduler->Run<LargeTask3>( Tuple{uint2{x,y}, 0u} );
			
			const ulong	required = grid_size * grid_size;
			
			for (;;)
			{
				if ( task_complete.load( EMemoryOrder::Relaxed ) >= required )
					break;

				scheduler->ProcessTask( EThread::Worker, 0 );
			}
			
			const nanoseconds	total_time	= TimePoint_t::clock::now() - start_time;
			const nanoseconds	total_time2	= total_time * (num_threads + 1);
			const double		overhead	= double(total_time2.count() - task_payload_time.load()) / double(total_time2.count());
			const ulong			task_count	= task_counter.load();

			AE_LOGI( "Total time: "s << ToString( total_time ) <<
					 ", final jobs: " << ToString( task_counter ) <<
					 ", task time: " << ToString( nanoseconds{ task_payload_time.load() / task_counter }) <<
					 ", overhead: " << ToString( overhead * 100.0, 2 ) << " %" );
		}
	}
}


extern void PerfTest_Threading ()
{
	for (uint i = 0; i < 4; ++i) {
		Threading_Test1();
		std::this_thread::sleep_for( seconds{1} );
	}

	AE_LOGI( "------------------------" );
	for (uint i = 0; i < 4; ++i) {
		Threading_Test2();
		std::this_thread::sleep_for( seconds{1} );
	}
	
	AE_LOGI( "------------------------" );
	for (uint i = 0; i < 4; ++i) {
		Threading_Test3();
		std::this_thread::sleep_for( seconds{1} );
	}

	AE_LOGI( "PerfTest_Threading - passed" );
}
