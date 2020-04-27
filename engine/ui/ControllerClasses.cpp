// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ui/ControllerClasses.h"
#include "ui/LayoutClasses.h"

#include "serializing/Serializer.h"
#include "serializing/Deserializer.h"

namespace AE::UI
{

/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool ButtonController::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _onTap, _onDoubleTap );
	}

	bool ButtonController::Deserialize (Serializing::Deserializer const& des)
	{
		return des( _onTap, _onDoubleTap );
	}

/*
=================================================
	Update
=================================================
*/
	void ButtonController::Update (InputState const &input, UIActionMap const& actionMap, ILayout *layout)
	{
		auto&	state = EditLayoutState( layout );
		ASSERT( not state.IsCulled() );
		
		EStyleState	new_state = Default;

		if ( auto* data = input.TryToCaptureCursor( this, state.GlobalRect(), false ))
		{
			new_state = (data->IsPressed() ? EStyleState::TouchDown : EStyleState::MouseOver);
			
			for (auto& gesture : data->Gestures())
			{
				switch ( gesture.type )
				{
					case InputState::EGesture::SingleTap :	actionMap.Call( _onTap, layout );		break;
					case InputState::EGesture::DoubleTap :	actionMap.Call( _onDoubleTap, layout );	break;
				}
			}
		}

		state.SetStyle( new_state );
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool SelectionController::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _onSelectionChanged, _selected );
	}

	bool SelectionController::Deserialize (Serializing::Deserializer const& des)
	{
		return des( _onSelectionChanged, _selected );
	}

/*
=================================================
	Update
=================================================
*/
	void SelectionController::Update (InputState const &input, UIActionMap const& actionMap, ILayout *layout)
	{
		auto&	state = EditLayoutState( layout );
		ASSERT( not state.IsCulled() );

		EStyleState	new_state = Default;

		if ( auto* data = input.TryToCaptureCursor( this, state.GlobalRect(), false ))
		{
			new_state |= (data->IsPressed() ? EStyleState::TouchDown : EStyleState::MouseOver);
			
			for (auto& gesture : data->Gestures())
			{
				if ( gesture.type == InputState::EGesture::SingleTap )
				{
					_selected = not _selected;
					actionMap.Call( _onSelectionChanged, layout, _selected );
				}
			}
		}

		if ( _selected ) new_state |= EStyleState::Selected;

		state.SetStyle( new_state );
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool DraggableController::Serialize (Serializing::Serializer &) const
	{
		return true;
	}

	bool DraggableController::Deserialize (Serializing::Deserializer const&)
	{
		return true;
	}

/*
=================================================
	Update
=================================================
*/
	void DraggableController::Update (InputState const &input, UIActionMap const&, ILayout *layout)
	{
		auto&	state		 = EditLayoutState( layout );
		auto*	fixed_layout = layout->As<FixedLayout>();

		ASSERT( fixed_layout );
		ASSERT( not state.IsCulled() );

		EStyleState	new_state = Default;

		if ( auto* data = input.TryToCaptureCursor( this, state.GlobalRect(), _isDragging ))
		{
			new_state |= (data->IsPressed() ? EStyleState::TouchDown : EStyleState::MouseOver);
			
			for (auto& gesture : data->Gestures())
			{
				switch ( gesture.type )
				{
					case InputState::EGesture::Dragging_Start :
						ASSERT( not _isDragging );
						_isDragging = true;
						fixed_layout->Move( (gesture.dragging.pos - gesture.dragging.prev) * 0.5f );
						break;

					case InputState::EGesture::Dragging_Update :
						if ( _isDragging ) {
							fixed_layout->Move( (gesture.dragging.pos - gesture.dragging.prev) * 0.5f );
						}
						break;

					case InputState::EGesture::Dragging_Stop :
						if ( _isDragging ) {
							_isDragging = false;
							fixed_layout->Move( (gesture.dragging.pos - gesture.dragging.prev) * 0.5f );	// TODO: temp
						}
						break;
				}
			}
		}

		if ( _isDragging ) new_state |= EStyleState::Selected;

		state.SetStyle( new_state );
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool ResizableController::Serialize (Serializing::Serializer &) const
	{
		return true;
	}

	bool ResizableController::Deserialize (Serializing::Deserializer const&)
	{
		return true;
	}

/*
=================================================
	Update
=================================================
*/
	void ResizableController::Update (InputState const &input, UIActionMap const&, ILayout *layout)
	{
		const auto	GetEdge = [] (const RectF &rect, const float2 &pos)
		{
			const float		border	= 8.0f;	// dips
			ELayoutAlign	dir		= Default;

			if ( Abs(rect.left  - pos.x) <= border )	dir |= ELayoutAlign::Left;	else
			if ( Abs(rect.right - pos.x) <= border )	dir |= ELayoutAlign::Right;

			if ( Abs(rect.top    - pos.y) <= border )	dir |= ELayoutAlign::Top;	else
			if ( Abs(rect.bottom - pos.y) <= border )	dir |= ELayoutAlign::Bottom;

			return dir;
		};

		const auto	Resize = [] (ELayoutAlign dir, FixedLayout &flayout, const float2 &delta)
		{
			RectF	reg = flayout.GetRegion();

			if ( EnumEq( dir, ELayoutAlign::Left ))		reg.left  += delta.x;
			if ( EnumEq( dir, ELayoutAlign::Right ))	reg.right += delta.x;

			if ( EnumEq( dir, ELayoutAlign::Top ))		reg.top    += delta.y;
			if ( EnumEq( dir, ELayoutAlign::Bottom ))	reg.bottom += delta.y;

			flayout.SetRegion( reg.Normalize() );
		};


		auto&	state		 = EditLayoutState( layout );
		auto*	fixed_layout = layout->As<FixedLayout>();

		ASSERT( fixed_layout );
		ASSERT( not state.IsCulled() );

		EStyleState	new_state  = Default;
		bool		is_started = (_direction != Default);

		if ( auto* data = input.TryToCaptureCursor( this, state.GlobalRect(), is_started ))
		{
			if ( data->IsPressed() )
				new_state |= EStyleState::TouchDown;
			else
			if ( not is_started and GetEdge( state.GlobalRect(), data->Position() ) != Default )
				new_state |= EStyleState::MouseOver;
			
			for (auto& gesture : data->Gestures())
			{
				switch ( gesture.type )
				{
					case InputState::EGesture::Dragging_Start :
						ASSERT( not is_started );
						_direction = GetEdge( state.GlobalRect(), data->Position() );
						Resize( _direction, *fixed_layout, gesture.dragging.pos - gesture.dragging.prev );
						break;

					case InputState::EGesture::Dragging_Update :
						Resize( _direction, *fixed_layout, gesture.dragging.pos - gesture.dragging.prev );
						break;

					case InputState::EGesture::Dragging_Stop :
						Resize( _direction, *fixed_layout, gesture.dragging.pos - gesture.dragging.prev );
						_direction = Default;
						break;
				}
			}
		}

		if ( is_started ) new_state |= EStyleState::Selected;

		state.SetStyle( new_state );
	}
//-----------------------------------------------------------------------------


}	// AE::UI
