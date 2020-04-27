// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ui/Widget.h"
#include "ui/Drawable.h"
#include "ui/Controller.h"
#include "ui/GestureRecognizer.h"

namespace AE::UI
{

	//
	// UI System
	//

	class UISystem
	{
	// types
	private:
		using ViewStack_t	= Array< WidgetPtr >;
		using TimePoint_t	= std::chrono::high_resolution_clock::time_point;

		using ObjectFactory		= Serializing::ObjectFactory;
		using IResourceManager	= Graphics::IResourceManager;
		using ITransferContext	= Graphics::ITransferContext;
		using IRenderContext	= Graphics::IRenderContext;


	// variables
	private:
		UniquePtr<Canvas>		_canvas;
		FontManagerPtr			_fontMngr;

		ViewStack_t				_stack;
		
		ILayout::UpdateQueue	_updateQueue;
		LayoutState				_viewState;
		TimePoint_t				_lastUpdate;
		UIActionMap				_actionMap;
		GestureRecognizer		_gestureRecognizer;

		Ptr<IResourceManager>	_resourceMngr;
		Ptr<ObjectFactory>		_objFactory;
		
		RWDataRaceCheck			_drCheck;


	// methods
	public:
		explicit UISystem ();

		bool  Add (const WidgetPtr &);
		bool  Remove (const WidgetPtr &);
		
		template <typename FN>
		bool  Register (const UIActionName &act, FN &&fn);

		bool  Create (const FontManagerPtr &fontMngr, IResourceManager &resMngr, ObjectFactory &factory);
		void  Destroy ();
		
		bool  Update ();
		void  PreDraw (ITransferContext &ctx);
		void  Draw (IRenderContext &ctx);

		bool  Resize (const uint2 &surfaceSizeInPix, const float2 &dipsPerPixel, const float2 &mmPerPixel);
		void  ProcessEvents (App::IWindow &);


	private:
		void  _RegisterLayouts ();
		void  _RegisterDrawables ();
		void  _RegisterControllers ();
	};
	


	struct UIRendererData
	{
		Canvas &			canvas;
		FontManager &		fontMngr;
		IResourceManager &	resourceMngr;
	};

	
	template <typename FN>
	inline bool  UISystem::Register (const UIActionName &act, FN &&fn)
	{
		return _actionMap.Register( act, std::forward<FN &&>( fn ));
	}

}	// AE::UI
