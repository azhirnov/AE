// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ui/Layout.h"
#include "ui/Drawable.h"
#include "ui/Controller.h"

namespace AE::UI
{

/*
=================================================
	Crop
----
	returns rectangle intersection or point of 'other'
=================================================
*/
	ND_ inline RectF  Crop (const RectF &self, const RectF &other)
	{
		ASSERT( self.IsNormalized() and other.IsNormalized() );

		RectF	res{ Max( self.left,   other.left ),
					 Max( self.top,    other.top ),
					 Min( self.right,  other.right ),
					 Min( self.bottom, other.bottom )};

		return res.IsNormalized() ? res : RectF{other.LeftTop(), other.LeftTop()};
	}
	
/*
=================================================
	constructor
=================================================
*/
	LayoutState::LayoutState (const RectF &localRect, EStyleState style)
	{
		_local		= localRect;
		_globalPos	= localRect.LeftTop();
		_clipped	= GlobalRect();
		_style		= style;
	}

/*
=================================================
	Update
=================================================
*/
	void LayoutState::Update (const LayoutState &parentState, const RectF &localRect)
	{
		// inherit some states from parent
		_style		|= (parentState.StyleFlags() & EStyleState::_Inherited);
		
		_local		= localRect;
		_globalPos	= _local.LeftTop() + parentState.GlobalRect().LeftTop();
		_clipped	= Crop( parentState.ClippedRect(), GlobalRect() );
	}
	
/*
=================================================
	UpdateAndFitParent
=================================================
*/
	void LayoutState::UpdateAndFitParent (const LayoutState &parentState, RectF localRect)
	{
		const float2	off = parentState.LocalRect().LeftTop();

		localRect = Crop( parentState.LocalRect(), localRect + off ) - off;
		
		return Update( parentState, localRect );
	}
	
/*
=================================================
	UpdateAndFillParent
=================================================
*/
	void LayoutState::UpdateAndFillParent (const LayoutState &parentState)
	{
		return Update( parentState, RectF{parentState.LocalRect().Size()} );
	}
	
/*
=================================================
	Resize
=================================================
*/
	void LayoutState::Resize (const RectF &localRect)
	{
		const float2	global_off	= _globalPos - _local.LeftTop();
		const RectF		clipped		= _clipped;

		_local		= localRect;
		_globalPos	= _local.LeftTop() + global_off;
		_clipped	= Crop( clipped, GlobalRect() );
	}
	
/*
=================================================
	ResizeAndFitPrevious
=================================================
*/
	void LayoutState::ResizeAndFitPrevious (RectF localRect)
	{
		const float2	global_off	= _globalPos - _local.LeftTop();
		const RectF		clipped		= _clipped;

		_local		= Crop( _local, localRect );
		_globalPos	= _local.LeftTop() + global_off;
		_clipped	= Crop( clipped, GlobalRect() );
	}
	
/*
=================================================
	SetStyle
=================================================
*/
	void LayoutState::SetStyle (EStyleState value)
	{
		_style = value;
	}
//-----------------------------------------------------------------------------
	
	

/*
=================================================
	destructor
=================================================
*/
	ILayout::UpdateQueue::~UpdateQueue ()
	{
	}

/*
=================================================
	Clear
=================================================
*/
	void ILayout::UpdateQueue::Clear ()
	{
		_pendingLayouts.clear();
		_tempLayouts.clear();
		_callbacks.clear();
		_drawables.clear();
		_controllers.clear();
	}
	
/*
=================================================
	EnqueueLayout
=================================================
*/
	void ILayout::UpdateQueue::EnqueueLayout (ILayout const *parent, ILayout* layout) const
	{
		if ( layout )
			_tempLayouts.push_back({ parent, layout });
	}

/*
=================================================
	EnqueueCallback
=================================================
*/
	void ILayout::UpdateQueue::EnqueueCallback (ILayout const *parent, ILayout* layout, UpdateFunc_t func) const
	{
		if ( not layout )
			return;

		auto	tmp = New<_LayoutCallback>( layout, func );

		_tempLayouts.push_back({ parent, tmp });
		_callbacks.push_back( _CallbackPtr_t{tmp} );
	}
	
/*
=================================================
	FlushQueue
=================================================
*/
	void ILayout::UpdateQueue::FlushQueue ()
	{
		_pendingLayouts.insert( _pendingLayouts.begin(), _tempLayouts.begin(), _tempLayouts.end() );
		_tempLayouts.clear();
	}
	
/*
=================================================
	AppendDrawable
=================================================
*/
	void ILayout::UpdateQueue::AppendDrawable (ILayout const *layout, IDrawable* drawable) const
	{
		if ( drawable )
			_drawables.push_back({ layout, drawable });
	}
	
/*
=================================================
	AppendController
=================================================
*/
	void ILayout::UpdateQueue::AppendController (ILayout *layout, IController* controller) const
	{
		if ( controller )
			_controllers.push_back({ layout, controller });
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	ILayout::ILayout ()
	{}
	
/*
=================================================
	destructor
=================================================
*/
	ILayout::~ILayout ()
	{}

/*
=================================================
	MarkAsDeleted
=================================================
*/
	void ILayout::MarkAsDeleted ()
	{
		_isDeleted = true;
		// TODO
	}
	
/*
=================================================
	GetPreferredSize
=================================================
*/
	RectF ILayout::GetPreferredSize (const RectF &parentRect) const
	{
		return RectF( parentRect.Size() );
	}
//-----------------------------------------------------------------------------


}	// AE::UI
