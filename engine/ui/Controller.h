// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "serializing/ISerializable.h"
#include "stl/Containers/AnyTypeRef.h"
#include "stl/Utils/NamedID_HashCollisionCheck.h"
#include "ui/Common.h"

namespace AE::UI
{
	struct LayoutState;
	class ILayout;


	//
	// Controller interface
	//

	class IController : public Serializing::ISerializable
	{
	// interface
	public:
		virtual ~IController () {}
		virtual void Update (struct InputState const&, struct UIActionMap const&, ILayout *) = 0;

	protected:
		ND_ static LayoutState&  EditLayoutState (ILayout *);
	};



	//
	// UI Action Map
	//

	struct UIActionMap
	{
	private:
		using Callback_t	= Function< void (const ILayout *, AnyTypeRef) >;
		using Map_t			= HashMap< UIActionID, Callback_t >;

		mutable std::shared_mutex	_guard;
		Map_t						_actions;

		DEBUG_ONLY(
			NamedID_HashCollisionCheck	_hcCheck;
		)


	public:
		UIActionMap () {}

		template <typename FN>
		bool Register (const UIActionID &act, FN &&fn);
		
	#ifndef AE_OPTIMIZE_IDS
		template <typename FN>
		bool Register (const UIActionName &act, FN &&fn);
	#endif

		void Call (const UIActionID &act, const ILayout *layout, AnyTypeRef data = {}) const;
	};



	//
	// Input State
	//

	struct InputState
	{
		friend class GestureRecognizer;

	// types
	public:
		enum class EGesture
		{
			Unknown,

			SingleTap,
			DoubleTap,
			LongPress,
			
			//Pinch	
			
			Rotation_Start,
			Rotation_Update,
			Rotation_Stop,

			Dragging_Start,
			Dragging_Update,
			Dragging_Stop,
		};

		struct TapData {
			float2		pos;	// dips
		};

		struct RotationData {
		};

		struct DraggingData {
			float2		prev;	// dips
			float2		pos;	// dips
		};

		struct GestureData
		{
			EGesture		type	= Default;
			union {
				TapData			singleTap;
				TapData			doubleTap;
				TapData			longPress;
				RotationData	rotation;
				DraggingData	dragging;
			};

			GestureData () {}
			GestureData (const GestureData &);
		};
		

	private:
		struct CursorData
		{
			friend struct InputState;

		private:
			mutable IController*	_focused		= null;
			float2					_position;		// dips
			bool					_pressed		= false;
			mutable bool			_resetFocus		= false;
			Array<GestureData>		_gestures;

		public:
				void					ResetFocus ()	const	{ _resetFocus = true; }
			ND_ float2 const&			Position ()		const	{ return _position; }
			ND_ bool					IsPressed ()	const	{ return _pressed; }
			ND_ ArrayView<GestureData>	Gestures ()		const	{ return _gestures; }
		};


	// variables
	private:
		CursorData		_cursor;


	// methods
	public:
		InputState () {}

		// for mouse and touches
		ND_ CursorData const*	TryToCaptureCursor (IController *, const RectF &, bool hasActiveEvents) const;
			bool				ReleaseCursor (IController *) const;

		// for keys
		//ND_ bool  TryToSetFocus (IController *, const RectF &, bool hasActiveEvents) const;
		//	bool  ResetFocus (IController *) const;

		// utils
			void SetCursorState (const float2 &pos, bool pressed);
			void BeforeUpdate ();

			void AddGesture (const GestureData &data);
			void ClearGestures ();
	};
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	Register
=================================================
*/
	template <typename FN>
	inline bool  UIActionMap::Register (const UIActionID &act, FN &&fn)
	{
		EXLOCK( _guard );
		CHECK_ERR( act.IsDefined() );
		CHECK_ERR( _actions.insert_or_assign( act, std::forward<FN &&>( fn )).second );
		return true;
	}
	
/*
=================================================
	Register
=================================================
*/
#ifndef AE_OPTIMIZE_IDS
	template <typename FN>
	inline bool  UIActionMap::Register (const UIActionName &act, FN &&fn)
	{
		EXLOCK( _guard );
		CHECK_ERR( act.IsDefined() );
		DEBUG_ONLY( _hcCheck.Add( act ));
		CHECK_ERR( _actions.insert_or_assign( UIActionID{act}, std::forward<FN &&>( fn )).second );
		return true;
	}
#endif

/*
=================================================
	Call
=================================================
*/
	inline void  UIActionMap::Call (const UIActionID &act, const ILayout *layout, AnyTypeRef data) const
	{
		if ( act.IsDefined() )
		{
			SHAREDLOCK( _guard );

			auto	iter = _actions.find( act );
			CHECK_ERR( iter != _actions.end(), void());

			return iter->second( layout, data );
		}
	}
//-----------------------------------------------------------------------------


	
/*
=================================================
	TryToCaptureCursor
=================================================
*/
	forceinline InputState::CursorData const*  InputState::TryToCaptureCursor (IController* current, const RectF &rect, bool hasActiveEvents) const
	{
		if ( (_cursor._focused == current) | (_cursor._focused == null) )
		{
			if ( rect.Intersects( _cursor._position ) | hasActiveEvents )
			{
				_cursor._resetFocus = not hasActiveEvents;
				_cursor._focused	= current;
				return &_cursor;
			}
			
			_cursor._resetFocus |= (_cursor._focused == current);
		}
		return null;
	}
	
/*
=================================================
	ReleaseCursor
=================================================
*/
	forceinline bool  InputState::ReleaseCursor (IController *current) const
	{
		if ( _cursor._focused == current )
		{
			_cursor._resetFocus = true;
			return true;
		}
		return false;
	}


}	// AE::UI
