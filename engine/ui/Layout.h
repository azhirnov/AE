// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'
/*
	Layout classes.

	Frame rendering order:
	- preparing
		- update controllers from childs to root
		- update all layouts from root to childs
		- after update from childs to root (if added to queue)
		- TODO: layout animation

	- drawing
		- draw drawables from root to childs
*/

#pragma once

#include "ui/LayoutEnums.h"
#include "serializing/ISerializable.h"
#include <typeindex>

namespace AE::UI
{
	using LayoutPtr				= UniquePtr< class ILayout >;
	using DrawablePtr			= UniquePtr< class IDrawable >;
	using ControllerPtr			= UniquePtr< class IController >;
	using LayoutAnimationPtr	= UniquePtr< class ILayoutAnimation >;

	

	//
	// Layout State
	//
	struct LayoutState
	{
	// variables
	private:
		float2			_globalPos;
		RectF			_clipped;
		RectF			_local;
		EStyleState		_style		= Default;
		
	// methods
	public:
		LayoutState () {}
		LayoutState (const RectF &localRect, EStyleState style);

		ND_ RectF const		GlobalRect ()	const	{ return RectF{ _local.Size() } + _globalPos; }
		ND_ RectF const&	LocalRect ()	const	{ return _local; }
		ND_ RectF const&	ClippedRect ()	const	{ return _clipped; }
		ND_ EStyleState		StyleFlags ()	const	{ return _style; }

		ND_ bool			IsCulled ()		const	{ return All(Equals( _clipped.Size(), float2{} )); }

		void Update (const LayoutState &parentState, const RectF &localRect);
		void UpdateAndFitParent (const LayoutState &parentState, RectF localRect);
		void UpdateAndFillParent (const LayoutState &parentState);

		void Resize (const RectF &localRect);
		void ResizeAndFitPrevious (RectF localRect);
		void SetStyle (EStyleState value);
	};



	//
	// Layout interface
	//
	class ILayout : public Serializing::ISerializable
	{
		friend class IController;

	// types
	public:
		struct UpdateQueue;


	// variables
	private:
		LayoutState		_state;
		bool			_isDeleted	= false;


	// methods
	protected:
		ILayout ();

		void MarkAsDeleted ();

		ND_ LayoutState&		EditState ()				{ return _state; }
		

	public:
		ND_ LayoutState const&	State ()			const	{ return _state; }

		ND_ bool				IsDeleted ()		const	{ return _isDeleted; }
		ND_ bool				IsAlive ()			const	{ return not _isDeleted; }

		template <typename T>	ND_ bool	 Is ()	const	{ return std::type_index{typeid(T)} == GetTypeId(); }
		template <typename T>	ND_ T *		 As ()			{ return Is<T>() ? Cast<T>( this ) : null; }
		template <typename T>	ND_ T const* As ()	const	{ return Is<T>() ? Cast<T const>( this ) : null; }


	// interface
	public:
			virtual ~ILayout ();

		ND_ virtual StringView Name () const = 0;

		ND_ virtual RectF GetPreferredSize (const RectF &parentRect) const;

			virtual void Update (const UpdateQueue &queue, const LayoutState &parentState) = 0;

			virtual bool AddChild (LayoutPtr &&) = 0;
			virtual bool SetDrawable (DrawablePtr &&) = 0;
			virtual bool SetController (ControllerPtr &&) = 0;
		//	virtual bool SetAnimation (LayoutAnimationPtr &&) = 0;
		ND_ virtual std::type_index GetTypeId () const = 0;
	};



	//
	// Update Queue
	//
	struct ILayout::UpdateQueue
	{
	// types
	public:
		using UpdateFunc_t		= void (*) (ILayout*, const UpdateQueue &, const LayoutState &);
		using LayoutQueue_t		= Deque<Pair< ILayout const *, ILayout *>>;				// TODO: use FIFO queue
		using TempLayouts_t		= Array<Pair< ILayout const *, ILayout *>>;
		using DrawableQueue_t	= Array<Pair< ILayout const *, IDrawable *>>;
		using ControllerQueue_t	= Array<Pair< ILayout *, IController *>>;

	private:
		class _LayoutCallback;
		using _CallbackPtr_t	= UniquePtr< _LayoutCallback >;
		using _Callbacks_t		= Array< _CallbackPtr_t >;


	// variables
	private:
		mutable LayoutQueue_t		_pendingLayouts;
		mutable LayoutQueue_t		_tempLayouts;				// small buffer for current layout, will be merged to '_pendingLayouts'
		mutable _Callbacks_t		_callbacks;					// array for temporary layouts which transfer update func call.

		mutable DrawableQueue_t		_drawables;
		mutable ControllerQueue_t	_controllers;
		bool						_forceUpdate	= true;		// if 'false' then layout can optimize self updating,
																// otherwise layout will totaly recalculated and all childs will be updated.


	// methods
	public:
		UpdateQueue () {}
		~UpdateQueue ();

			void EnqueueLayout (ILayout const *parent, ILayout* layout) const;
			void EnqueueCallback (ILayout const *p, ILayout* l, UpdateFunc_t f) const;

			void FlushQueue ();
		ND_ auto ExtractLayout ();

			void AppendDrawable (ILayout const *layout, IDrawable* drawable) const;
			void AppendController (ILayout *layout, IController* controller) const;

			void Clear ();

			bool IsForceUpdate () const				{ return _forceUpdate; }
			void SetForceUpdate (bool value)		{ _forceUpdate = value; }

		ND_ bool HasLayouts () const				{ return _pendingLayouts.empty(); }

		ND_ DrawableQueue_t&	Drawables ()		{ return _drawables; }
		ND_ ControllerQueue_t&	Controllers ()		{ return _controllers; }
	};


	
	//
	// Layout Callback
	//
	class ILayout::UpdateQueue::_LayoutCallback final : public ILayout
	{
	// variables
	private:
		ILayout *		_self;
		UpdateFunc_t	_func;

	// methods
	public:
		_LayoutCallback (ILayout* self, UpdateFunc_t func) : _self(self), _func(func)
		{}
		
		bool Serialize (Serializing::Serializer &) const override { return false; }

		bool Deserialize (Serializing::Deserializer const&) override { return false; }

		void Update (const UpdateQueue &queue, const LayoutState &parentState) override
		{
			_func( _self, queue, parentState );
		}
		
		ND_ StringView Name () const override				{ return {}; }

			bool AddChild (LayoutPtr &&) override			{ return false; }
			bool SetDrawable (DrawablePtr &&) override		{ return false; }
			bool SetController (ControllerPtr &&) override	{ return false; }

		ND_ std::type_index  GetTypeId () const override	{ return typeid( *this ); }
	};
	
	
/*
=================================================
	ExtractLayout
=================================================
*/
	inline auto ILayout::UpdateQueue::ExtractLayout ()
	{
		auto res{ std::move(_pendingLayouts.front()) };
		_pendingLayouts.pop_front();
		return res;
	}


}	// AE::UI
