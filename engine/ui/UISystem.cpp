// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ui/UISystem.h"
#include "ui/Drawable.h"
#include "ui/Controller.h"
#include "ui/LayoutClasses.h"
#include "ui/DrawableClasses.h"
#include "ui/ControllerClasses.h"
#include "serializing/ObjectFactory.h"
#include "platform/Public/IWindow.h"

namespace AE::UI
{
	using SerializedID = Serializing::SerializedID;

/*
=================================================
	constructor
=================================================
*/
	UISystem::UISystem () :
		_viewState{ RectF{float2(800.0f, 600.0f)}, Default }
	{
		_stack.reserve( 32 );
	}
	
/*
=================================================
	Add
=================================================
*/
	bool  UISystem::Add (const WidgetPtr &ptr)
	{
		EXLOCK( _drCheck );
		ASSERT( not Remove( ptr ));

		_stack.push_back( ptr );
		return true;
	}
	
/*
=================================================
	Remove
=================================================
*/
	bool  UISystem::Remove (const WidgetPtr &ptr)
	{
		EXLOCK( _drCheck );

		for (auto iter = _stack.begin(); iter != _stack.end(); ++iter)
		{
			if ( *iter == ptr )
			{
				_stack.erase( iter );
				return true;
			}
		}
		return false;
	}
	
/*
=================================================
	Create
=================================================
*/
	bool  UISystem::Create (const FontManagerPtr &fontMngr, IResourceManager &resMngr, ObjectFactory &factory)
	{
		EXLOCK( _drCheck );

		CHECK_ERR( not _objFactory );
		CHECK_ERR( not _fontMngr );
		CHECK_ERR( not _canvas );
		CHECK_ERR( fontMngr );

		_canvas.reset( New<Canvas>( resMngr ));
		_fontMngr		= fontMngr;
		_objFactory		= &factory;
		_resourceMngr	= &resMngr;
		_lastUpdate		= TimePoint_t::clock::now();

		_RegisterLayouts();
		_RegisterDrawables();
		_RegisterControllers();

		return true;
	}
	
/*
=================================================
	Destroy
=================================================
*/
	void  UISystem::Destroy ()
	{
		EXLOCK( _drCheck );

		_updateQueue.Clear();
		_stack.clear();
		
		if ( _canvas )
		{
			_canvas->Clear();
			_canvas.reset();
		}

		_fontMngr		= null;
		_objFactory		= null;
		_resourceMngr	= null;
	}

/*
=================================================
	Update
=================================================
*/
	bool  UISystem::Update ()
	{
		using namespace std::chrono;
		using Seconds = duration<float>;

		EXLOCK( _drCheck );
		
		// update time
		const auto	old_time = _lastUpdate;
		_lastUpdate = TimePoint_t::clock::now();
		const float	dt = duration_cast<Seconds>( _lastUpdate - old_time ).count();


		// update controllers from previous frame
		_gestureRecognizer.BeforeUpdate();
		for (auto& cont : Reverse(_updateQueue.Controllers()))
		{
			cont.second->Update( _gestureRecognizer.GetInputState(), _actionMap, cont.first );
		}
		_gestureRecognizer.AfterUpdate();
		_updateQueue.Clear();


		// update layouts
		for (auto iter = _stack.rbegin(); iter != _stack.rend(); ++iter)
		{
			(*iter)->Update( INOUT _updateQueue );

			while ( not _updateQueue.HasLayouts() )
			{
				auto		curr	= _updateQueue.ExtractLayout();
				auto const&	state	= curr.first ? curr.first->State() : _viewState;
			
				curr.second->Update( _updateQueue, state );
				_updateQueue.FlushQueue();
			}

			if ( (*iter)->IsOpaque() )
				break;
		}

		// draw ui
		UIRendererData	renderer{ *_canvas, *_fontMngr, *_resourceMngr };

		for (auto& draw : _updateQueue.Drawables())
		{
			draw.second->Draw( renderer, draw.first->State(), dt );
		}

		return true;
	}
	
/*
=================================================
	PreDraw
=================================================
*/
	void  UISystem::PreDraw (ITransferContext &ctx)
	{
		EXLOCK( _drCheck );

		CHECK( _canvas->Flush( ctx ));
		CHECK( _fontMngr->Flush( ctx ));
	}

/*
=================================================
	Draw
=================================================
*/
	void  UISystem::Draw (IRenderContext &ctx)
	{
		EXLOCK( _drCheck );

		_canvas->Draw( ctx );
	}

/*
=================================================
	Resize
=================================================
*/
	bool  UISystem::Resize (const uint2 &surfaceSizeInPix, const float2 &dipsPerPixel, const float2 &mmPerPixel)
	{
		EXLOCK( _drCheck );

		_canvas->SetDimensions( surfaceSizeInPix, dipsPerPixel, mmPerPixel );

		_viewState = LayoutState{ RectF{float2(surfaceSizeInPix)}, Default };
		return true;
	}
	
/*
=================================================
	ProcessEvents
=================================================
*/
	void  UISystem::ProcessEvents (App::IWindow &wnd)
	{
		_gestureRecognizer.ProcessEvents( *_canvas, wnd.GetInputEventQueue() );
	}
	
/*
=================================================
	_RegisterLayouts
=================================================
*/
	void  UISystem::_RegisterLayouts ()
	{
		_objFactory->Register<WidgetLayout>( SerializedID{"WidgetLayout"} );
		_objFactory->Register<AutoSizeLayout>( SerializedID{"AutoSizeLayout"} );
		_objFactory->Register<StackLayout>( SerializedID{"StackLayout"} );
		_objFactory->Register<FillStackLayout>( SerializedID{"FillStackLayout"} );
		_objFactory->Register<FixedLayout>( SerializedID{"FixedLayout"} );
		_objFactory->Register<AlignedLayout>( SerializedID{"AlignedLayout"} );
		_objFactory->Register<FixedMarginLayout>( SerializedID{"FixedMarginLayout"} );
		_objFactory->Register<RelativeMarginLayout>( SerializedID{"RelativeMarginLayout"} );
		_objFactory->Register<FixedPaddingLayout>( SerializedID{"FixedPaddingLayout"} );
		_objFactory->Register<RelativePaddingLayout>( SerializedID{"RelativePaddingLayout"} );
	}
	
/*
=================================================
	_RegisterDrawables
=================================================
*/
	void  UISystem::_RegisterDrawables ()
	{
		_objFactory->Register<RectangleDrawable>( SerializedID{"RectangleDrawable"} );
		_objFactory->Register<ImageDrawable>( SerializedID{"ImageDrawable"} );
		_objFactory->Register<NinePatchDrawable>( SerializedID{"NinePatchDrawable"} );
		_objFactory->Register<TextDrawable>( SerializedID{"TextDrawable"} );
	}
	
/*
=================================================
	_RegisterControllers
=================================================
*/
	void  UISystem::_RegisterControllers ()
	{
		_objFactory->Register<ButtonController>( SerializedID{"ButtonController"} );
		_objFactory->Register<SelectionController>( SerializedID{"SelectionController"} );
		_objFactory->Register<DraggableController>( SerializedID{"DraggableController"} );
		_objFactory->Register<ResizableController>( SerializedID{"ResizableController"} );
	}
	

}	// AE::UI
