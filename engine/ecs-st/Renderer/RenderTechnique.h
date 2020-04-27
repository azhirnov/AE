// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Renderer/IRenderPass.h"
#include "ecs-st/Core/Registry.h"
#include "ecs-st/Renderer/Components.h"
#include "stl/Math/Frustum.h"

namespace AE::ECS::Systems
{

	//
	// Render Technique
	//
	/*
	class RenderTechnique
	{
	private:
		IRenderPass*	depthPerPass;
		IRenderPass*	opaquePass;
		IRenderPass*	transparentPass;
		IRenderPass*	particleSym;

	public:
		void Setup ()
		{
			{
				RenderPassDesc	desc;

				SetupGraphicsPass( desc, INOUT depthPerPass );
			}
			{
				ComputePassDesc	desc;

				SetupComputePass( desc, INOUT particleSym );
			}
		}

		void Render ()
		{
			depthPerPass->Render();
			opaquePass->Render();
			transparentPass->Render();
		}
	};*/


	// visibility + renderer
	class ICameraSystem
	{
	public:
		struct RenderTask
		{
			struct {
				ImageID			image;
				ImageLayer		baseLayer;
				uint			layerCount	= 1;
				MipmapLevel		baseMipmap;
				uint			mipmapCount	= 1;
				RectU			region;				// TODO: 3d coords?
			}				in;
			struct {
				CommandBufferID	commands;
			}				out;
		};

	public:
		virtual bool  Execute (INOUT RenderTask &) const = 0;	// don't store any data for execution, it may be called many times per frame
		// Promise<RenderResult>  Execute () = 0; // for mt version
	};


	//
	// Visibility System
	//
	class VisibilitySystem
	{
		using RenderTask = ICameraSystem::RenderTask;

	private:
		Registry&				_owner;
		FrustumTempl<float>		_frustum;

	public:
		VisibilitySystem (Registry &reg) : _owner{reg} {}

		void Execute (const RenderTask &task) const
		{
			// - find secondary cameras that are visible by primary
			// - 
		}
	};


	//
	// Renderer System
	//
	class RendererSystem
	{
		using RenderTask = ICameraSystem::RenderTask;

	private:
		Registry&		_owner;

	public:
		RendererSystem (Registry &reg) : _owner{reg} {}
		
		void Execute (INOUT RenderTask &task) const
		{
		}
	};


	//
	// Camera 3D System
	//
	class Camera3DSystem : public ICameraSystem
	{
	private:
		EntityID			_cameraEnt;
		Registry			_registry;
		Registry&			_owner;
		VisibilitySystem	_visibilitySys;
		RendererSystem		_rendererSys;


	public:
		Camera3DSystem (EntityID entId, Registry &reg) :
			_cameraEnt{entId}, _registry{}, _owner{reg},
			_visibilitySys{_registry}, _rendererSys{_registry}
		{
		}

		bool  Execute (INOUT RenderTask &task) const override
		{
			CHECK_ERR( task.image );

			auto	cam = _owner.GetComponent<const Components::Camera3D>( _cameraEnt );
			CHECK_ERR( cam );

			_visibilitySys.Execute( task );
			_rendererSys.Execute( INOUT task );

			return true;
		}
	};


	//
	// Ray Tracing Camera System
	//
	class RayTracingCameraSystem : public ICameraSystem
	{
	private:
		EntityID		_cameraEnt;
		Registry		_registry;		// for visibility & rendering objects
		Registry&		_owner;


	public:
		RayTracingCameraSystem (EntityID entId, Registry &reg) : _cameraEnt{entId}, _owner{reg}
		{
		}

		bool  Execute (INOUT RenderTask &task) const override
		{
			CHECK_ERR( task.image );

			auto	cam = _owner.GetComponent<const Components::Camera3D>( _cameraEnt );
			CHECK_ERR( cam );
			
			return true;
		}
	};


	//
	// Hybrid Render Camera System
	//
	class HybridRenderCameraSystem : public Camera3DSystem
	{
	private:
		RayTracingCameraSystem	rayTracing;
		
	public:
		bool  Execute (INOUT RenderTask &task) const override
		{
			// run visibility systems
			// build render graph for rasterization & ray tracing
		}
	};


	//
	// Render Graph
	//
	class RenderGraph
	{
	private:
		using PrimaryCameraTag	= Components::CameraTag<Renderer::ECameraType::Primary>;
		using CameraSystemMap_t	= HashMap< EntityID, UniquePtr<ICameraSystem> >;
		using RenderTask		= ICameraSystem::RenderTask;


	private:
		Registry&			_owner;
		QueryID				_qPrimaryCameras;
		CameraSystemMap_t	_cameraSystems;


	public:
		RenderGraph (Registry &reg) : _owner{reg}
		{
			_qPrimaryCameras = _owner.CreateQuery< Require<PrimaryCameraTag> >();

			_owner.AddMessageListener<Components::CameraSystemTag, MsgTag_RemovedComponent>(
				[this] (ArrayView<EntityID> entities)
				{
					for (auto& id : entities) {
						_cameraSystems.erase( id );
					}
				});
		}

		void AddSystem (EntityID entId, UniquePtr<ICameraSystem> system)
		{
			CHECK_ERR( system, void());
			_owner.AssignComponent<Components::CameraSystemTag>( entId );

			_cameraSystems.insert_or_assign( entId, std::move(system) );
		}

		void Execute ()
		{
			// find primary cameras
			_owner.Execute( _qPrimaryCameras,
				[this] (ArrayView<Tuple< size_t, ReadAccess<EntityID>, ReadAccess<Components::Viewport>, Require<PrimaryCameraTag> >> chunks)
				{
					for (auto& chunk : chunks)
					{
						const size_t	count		= chunk.Get<0>();
						auto			entities	= chunk.Get<1>();
						auto			viewports	= chunk.Get<2>();

						for (size_t i = 0; i < count; ++i)
						{
							auto	iter = _cameraSystems.find( entities[i] );
							if ( iter != _cameraSystems.end() )
							{
								RenderTask	task;
								task.in.image		= viewports[i].image;
								task.in.baseLayer	= viewports[i].layer;
								task.in.baseMipmap	= viewports[i].mipmap;
								task.in.region		= viewports[i].region;

								CHECK( iter->second->Execute( task ));
							}
						}
					}
				});
		}
	};


}	// AE::ECS::Systems
