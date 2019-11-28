// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

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
	static siv::PerlinNoise			noise;
	static const uint				max_levels		= 6;
	static Atomic<uint64_t>			task_complete	{0};


	class HeightMap : public std::enable_shared_from_this<HeightMap>
	{
	public:
		static constexpr uint			W = 32;
		static constexpr uint			H = 32;
		StaticArray< uint8_t, W*H >		data;
	};
	using HeightMap_t = SharedPtr< HeightMap >;


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
			_cell{ cell }, _level{ level }, _result{ MakeShared<HeightMap>() }
		{}

	private:
		void Run () override
		{
			const double2	size	{ Pow( 0.5, double(_level) )};
			const double2	offset	= double2(_cell) * size;

			for (uint y = 0; y < HeightMap::H; ++y)
			for (uint x = 0; x < HeightMap::W; ++x)
			{
				double2	c = offset + (double2(x, y) / double2(HeightMap::W, HeightMap::H)) * size;
				noise.octaveNoise( c.x, c.y, 8 );
			}

			if ( _level < max_levels )
			{
				Scheduler().Run<LargeTask1>( _cell * 2 + uint2(0,0), _level+1 );
				Scheduler().Run<LargeTask1>( _cell * 2 + uint2(0,1), _level+1 );
				Scheduler().Run<LargeTask1>( _cell * 2 + uint2(1,0), _level+1 );
				Scheduler().Run<LargeTask1>( _cell * 2 + uint2(1,1), _level+1 );
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

	void Threading_Test1 ()
	{
		using TimePoint_t = std::chrono::high_resolution_clock::time_point;

		task_complete.store( 0 );

		const size_t		num_threads = std::thread::hardware_concurrency()-1;
		LocalTaskScheduler	scheduler	{num_threads};

		for (size_t i = 0; i < num_threads; ++i) {
			scheduler->AddThread( MakeShared<WorkerThread>(
				WorkerThread::ThreadMask{}.set(uint(WorkerThread::EThread::Worker)),
				WorkerThread::Milliseconds{i > 0 ? 10 : 0} // only one worker thread should never sleep
			));
		}

		const auto	start_time = TimePoint_t::clock::now();
		
		Unused(
			scheduler->Run<LargeTask1>( uint2{0,0}, 0 ),
			scheduler->Run<LargeTask1>( uint2{0,1}, 0 ),
			scheduler->Run<LargeTask1>( uint2{1,0}, 0 ),
			scheduler->Run<LargeTask1>( uint2{1,1}, 0 )
		);

		const uint64_t	required = 4 * (4ull << max_levels);

		for (;;)
		{
			if ( task_complete.load( EMemoryOrder::Relaxed ) >= required )
				break;

			std::this_thread::yield();
		}

		AE_LOGI( "Total time: "s << ToString( TimePoint_t::clock::now() - start_time ) << ", final jobs: " << ToString( required ) );
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
			_cell{ cell }, _level{ level }, _result{ MakeShared<HeightMap>() }
		{}

	private:
		void Run () override
		{
			const double2	size	{ Pow( 0.5, double(_level) )};
			const double2	offset	= double2(_cell) * size;

			for (uint y = 0; y < HeightMap::H; ++y)
			for (uint x = 0; x < HeightMap::W; ++x)
			{
				double2	c = offset + (double2(x, y) / double2(HeightMap::W, HeightMap::H)) * size;
				noise.octaveNoise( c.x, c.y, 8 );
			}

			if ( _level < max_levels )
			{
				auto	t0 = Scheduler().Run<LargeTask2>( _cell * 2 + uint2(0,0), _level+1 );
				auto	t1 = Scheduler().Run<LargeTask2>( StrongDeps{t0}, _cell * 2 + uint2(0,1), _level+1 );
				auto	t2 = Scheduler().Run<LargeTask2>( StrongDeps{t0, t1}, _cell * 2 + uint2(1,0), _level+1 );
				auto	t3 = Scheduler().Run<LargeTask2>( StrongDeps{t0, t1, t2}, _cell * 2 + uint2(1,1), _level+1 );
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

	void Threading_Test2 ()
	{
		using TimePoint_t = std::chrono::high_resolution_clock::time_point;

		task_complete.store( 0 );
		
		const size_t		num_threads = std::thread::hardware_concurrency()-1;
		LocalTaskScheduler	scheduler	{num_threads};

		for (size_t i = 0; i < num_threads; ++i) {
			scheduler->AddThread( MakeShared<WorkerThread>(
				WorkerThread::ThreadMask{}.set(uint(WorkerThread::EThread::Worker)),
				WorkerThread::Milliseconds{i > 0 ? 10 : 0} // only one worker thread should never sleep
			));
		}

		const auto	start_time = TimePoint_t::clock::now();
		
		Unused(
			scheduler->Run<LargeTask2>( uint2{0,0}, 0 ),
			scheduler->Run<LargeTask2>( uint2{0,1}, 0 ),
			scheduler->Run<LargeTask2>( uint2{1,0}, 0 ),
			scheduler->Run<LargeTask2>( uint2{1,1}, 0 )
		);

		const uint64_t	required = 4 * (4ull << max_levels);

		for (;;)
		{
			if ( task_complete.load( EMemoryOrder::Relaxed ) >= required )
				break;

			std::this_thread::yield();
		}

		AE_LOGI( "Total time: "s << ToString( TimePoint_t::clock::now() - start_time ) << ", final jobs: " << ToString( required ) );
	}
}


extern void PerfTest_Threading ()
{
	for (uint i = 0; i < 4; ++i) {
		Threading_Test1();
	}

	AE_LOGI( "------------------------" );
	for (uint i = 0; i < 4; ++i) {
		Threading_Test2();
	}

	AE_LOGI( "PerfTest_Threading - passed" );
}
