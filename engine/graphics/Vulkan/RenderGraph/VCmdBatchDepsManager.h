// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

namespace AE::Graphics
{

	//
	// Vulkan Command Batch Dependency Manager
	//

	class VCmdBatchDepsManager final : public Threading::ITaskDependencyManager
	{
	// types
	private:
		struct Dependency
		{
			CmdBatchID		batchId;
			AsyncTask		task;
			uint			bitIndex;

			Dependency (const CmdBatchID &batchId, const AsyncTask &task, uint index) : batchId{batchId}, task{task}, bitIndex{index} {}
		};


	// variables
	private:
		Mutex				_depsListGuard;		// TODO: lock-free ?
		Array<Dependency>	_depsList;			// TODO: ring-buffer or list for random erase, or map with CmdBatchID key


	// methods
	public:
		void  Update (VRenderGraph &rg);

		bool  Resolve (AnyTypeCRef dep, const AsyncTask &task, INOUT uint &bitIndex) override;
	};

	
/*
=================================================
	Update
=================================================
*/
	void  VCmdBatchDepsManager::Update (VRenderGraph &rg)
	{
		EXLOCK( _depsListGuard );

		for (auto iter = _depsList.begin(); iter != _depsList.end();)
		{
			if ( rg.IsComplete({ iter->batchId }) )
			{
				_SetDependencyCompletionStatus( iter->task, iter->bitIndex, false );
				iter = _depsList.erase( iter );
			}
			else
				++iter;
		}
	}

/*
=================================================
	Resolve
=================================================
*/
	bool  VCmdBatchDepsManager::Resolve (AnyTypeCRef dep, const AsyncTask &task, INOUT uint &bitIndex)
	{
		CHECK_ERR( dep.Is<CmdBatchDep>() );
		EXLOCK( _depsListGuard );

		_depsList.emplace_back( dep.As<CmdBatchDep>().id, task, bitIndex );
		++bitIndex;

		return true;
	}

}	// AE::Graphics
