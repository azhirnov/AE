// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ui/LayoutClasses.h"
#include "ui/Drawable.h"
#include "ui/Controller.h"
#include "serializing/ObjectFactory.h"

namespace AE::UI
{

	//
	// Dummy Layout
	//
	class DummyLayout : public ILayout
	{
	// methods
	protected:
		explicit DummyLayout () : ILayout() {}

		StringView Name () const override final					{ return {}; }

		bool AddChild (LayoutPtr &&) override final				{ return false; }
		bool SetDrawable (DrawablePtr &&) override final		{ return false; }
		bool SetController (ControllerPtr &&) override final	{ return false; }
	};
//-----------------------------------------------------------------------------


	
/*
=================================================
	constructor
=================================================
*/
	DefaultLayout::DefaultLayout ()
	{}

	DefaultLayout::DefaultLayout (StringView name) :
		ILayout(), _name( name )
	{}

	DefaultLayout::~DefaultLayout ()
	{}

/*
=================================================
	AddChild
=================================================
*/
	bool DefaultLayout::AddChild (LayoutPtr &&layout)
	{
		CHECK_ERR( not _child and layout );

		_child = std::move(layout);
		return true;
	}
	
/*
=================================================
	SetDrawable
=================================================
*/
	bool DefaultLayout::SetDrawable (DrawablePtr &&drawable)
	{
		CHECK_ERR( not _drawable );

		_drawable = std::move(drawable);
		return true;
	}
	
/*
=================================================
	SetController
=================================================
*/
	bool DefaultLayout::SetController (ControllerPtr &&controller)
	{
		CHECK_ERR( not _controller );

		_controller = std::move(controller);
		return true;
	}
//-----------------------------------------------------------------------------
	

	
/*
=================================================
	constructor
=================================================
*/
	DefaultArrayLayout::DefaultArrayLayout ()
	{}

	DefaultArrayLayout::DefaultArrayLayout (StringView name) :
		ILayout(), _name( name )
	{}

	DefaultArrayLayout::~DefaultArrayLayout ()
	{}

/*
=================================================
	AddChild
=================================================
*/
	bool DefaultArrayLayout::AddChild (LayoutPtr &&layout)
	{
		CHECK_ERR( layout );

		_childs.push_back( std::move(layout) );
		return true;
	}
	
/*
=================================================
	SetDrawable
=================================================
*/
	bool DefaultArrayLayout::SetDrawable (DrawablePtr &&drawable)
	{
		CHECK_ERR( not _drawable );

		_drawable = std::move(drawable);
		return true;
	}
	
/*
=================================================
	SetController
=================================================
*/
	bool DefaultArrayLayout::SetController (ControllerPtr &&controller)
	{
		CHECK_ERR( not _controller );

		_controller = std::move(controller);
		return true;
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
	constructor
=================================================
*/
	WidgetLayout::WidgetLayout (StringView name) : DefaultLayout( name )
	{}

/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool WidgetLayout::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _name );
	}

	bool WidgetLayout::Deserialize (Serializing::Deserializer const& des)
	{
		return des( _name );
	}
	
/*
=================================================
	Update
=================================================
*/
	void WidgetLayout::Update (const UpdateQueue &queue, const LayoutState &parentState)
	{
		EditState().UpdateAndFillParent( parentState );
		
		queue.EnqueueLayout( this, _child.get() );
		
		queue.AppendDrawable( this, _drawable.get() );
		queue.AppendController( this, _controller.get() );
	}
	
//-----------------------------------------------------------------------------
	
	

/*
=================================================
	constructor
=================================================
*/
	AutoSizeLayout::AutoSizeLayout (StringView name) : DefaultArrayLayout( name )
	{}

/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool AutoSizeLayout::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _name );
	}

	bool AutoSizeLayout::Deserialize (Serializing::Deserializer const& des)
	{
		return des( _name );
	}
	
/*
=================================================
	Update
=================================================
*/
	void AutoSizeLayout::Update (const UpdateQueue &queue, const LayoutState &parentState)
	{
		EditState().UpdateAndFillParent( parentState );
		
		queue.AppendDrawable( this, _drawable.get() );
		queue.AppendController( this, _controller.get() );

		for (auto& child : _childs)
		{
			queue.EnqueueLayout( this, child.get() );
		}

		queue.EnqueueCallback( this, this, &_AfterUpdate );
	}
	
/*
=================================================
	GetPreferredSize
=================================================
*/
	RectF AutoSizeLayout::GetPreferredSize (const RectF &parentRect) const
	{
		RectF const		max_region	= RectF( parentRect.Size() );
		const usize	count		= _childs.size();
		RectF			region		= count ? _childs.front()->GetPreferredSize( max_region ) : RectF{};

		for (usize i = 1; i < count; ++i)
		{
			region.Join( _childs[i]->GetPreferredSize( max_region ));
		}
		
		if ( _drawable )
		{
			RectF	rect;
			if ( _drawable->GetPreferredSize( max_region, OUT rect ))
				region.Join( rect );
		}

		return region;
	}

/*
=================================================
	_AfterUpdate
=================================================
*/
	void AutoSizeLayout::_AfterUpdate (ILayout *layout, const UpdateQueue &, const LayoutState &)
	{
		auto*			self		= Cast< AutoSizeLayout >( layout );
		RectF const		max_region	= self->State().LocalRect();
		const usize	count		= self->_childs.size();
		RectF			region		= count ? self->_childs.front()->State().LocalRect() : RectF{};

		for (usize i = 1; i < count; ++i)
		{
			region.Join( self->_childs[i]->State().LocalRect() );
		}
		
		if ( self->_drawable )
		{
			RectF	rect;
			if ( self->_drawable->GetPreferredSize( max_region, OUT rect ))
				region.Join( rect );
		}

		self->EditState().Resize( region );
	}
//-----------------------------------------------------------------------------


	
	//
	// Stack Layout Cell
	//
	class StackLayoutCell final : public DummyLayout
	{
	// variables
	protected:
		LayoutPtr		_child;
		RectF			_requiredRect;

	// methods
	public:
		StackLayoutCell (LayoutPtr &&child) : _child( std::move(child) )
		{}
		
		bool Serialize (Serializing::Serializer &) const override { return false; }

		bool Deserialize (Serializing::Deserializer const&) override { return false; }

		RectF GetPreferredSize (const RectF &parentSize) const override
		{
			return _child->GetPreferredSize( parentSize );
		}
		
		void Update (const UpdateQueue &queue, const LayoutState &parentState) override
		{
			queue.EnqueueLayout( this, _child.get() );

			EditState().Update( parentState, _requiredRect );
		}

		std::type_index  GetTypeId () const override	{ return typeid( *this ); }
		
		void SetRequiredRegion (const RectF &r)			{ ASSERT( r.IsNormalized() );  _requiredRect = r; }
	};

/*
=================================================
	constructor
=================================================
*/
	StackLayout::StackLayout (StringView name, EStackOrigin origin) :
		DefaultArrayLayout( name ), _origin( origin )
	{}
		
/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool StackLayout::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _name, _origin );
	}

	bool StackLayout::Deserialize (Serializing::Deserializer const& des)
	{
		return des( _name, _origin );
	}

/*
=================================================
	Update
=================================================
*/
	void StackLayout::Update (const UpdateQueue &queue, const LayoutState &parentState)
	{
		queue.AppendDrawable( this, _drawable.get() );
		queue.AppendController( this, _controller.get() );
		
		RectF const		max_size { parentState.LocalRect().Size() };
		float2			offset;
		RectF			local_rect;

		for (usize i = 0; i < _childs.size(); ++i)
		{
			queue.EnqueueLayout( this, _childs[i].get() );

			auto*	cell = Cast< StackLayoutCell >( _childs[i].get() );
			RectF	rect = RectF( cell->GetPreferredSize( max_size ).Size() );

			switch ( _origin )
			{
				case EStackOrigin::Left :
				{
					rect	 += float2{ offset.x, 0.0f };
					offset.x += rect.Width();
					break;
				}
				case EStackOrigin::Right :
				{
					rect	 += float2( max_size.right - offset.x, 0.0f );
					offset.x += rect.Width();
					break;
				}
				case EStackOrigin::Bottom :
				{
					rect	 += float2{ 0.0f, offset.y };
					offset.y += rect.Height();
					break;
				}
				case EStackOrigin::Top :
				{
					rect	 += float2( 0.0f, max_size.top - offset.y );
					offset.y += rect.Height();
					break;
				}
			}

			if ( i )	local_rect.Join( rect );
			else		local_rect = rect;

			cell->SetRequiredRegion( rect );
		}
		
		EditState().Update( parentState, local_rect );
	}
	
/*
=================================================
	GetPreferredSize
=================================================
*/
	RectF StackLayout::GetPreferredSize (const RectF &parentSize) const
	{
		return RectF( parentSize.Size() );
	}

/*
=================================================
	AddChild
=================================================
*/
	bool StackLayout::AddChild (LayoutPtr &&layout)
	{
		if ( layout->GetTypeId() == typeid(StackLayoutCell) )
		{
			_childs.push_back( std::move(layout) );
			return true;
		}

		_childs.push_back( LayoutPtr( New<StackLayoutCell>( std::move(layout) )) );
		return true;
	}
//-----------------------------------------------------------------------------

	
	
	//
	// Fill Stack Layout Cell
	//
	class FillStackLayoutCell : public DummyLayout
	{
	// variables
	protected:
		LayoutPtr		_child;
		RectF			_requiredRect;
		float			_weight;

	// methods
	public:
		FillStackLayoutCell (LayoutPtr &&child, float w) :
			_child( std::move(child) ), _weight( w )
		{}
		
		bool Serialize (Serializing::Serializer &) const override { return false; }

		bool Deserialize (Serializing::Deserializer const&) override { return false; }

		void Update (const UpdateQueue &queue, const LayoutState &parentState) override
		{
			queue.EnqueueLayout( this, _child.get() );

			EditState().Update( parentState, _requiredRect );
		}

		std::type_index  GetTypeId () const override		{ return typeid( *this ); }

		void SetRequiredRegion (const RectF &r)				{ ASSERT( r.IsNormalized() );  _requiredRect = r; }
		float Weight () const								{ return _weight; }
	};

/*
=================================================
	constructor
=================================================
*/
	FillStackLayout::FillStackLayout (StringView name, EStackOrigin origin, float spacing) :
		DefaultArrayLayout( name ), _origin( origin ), _spacing( spacing )
	{}
		
/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool FillStackLayout::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _name, _origin, _spacing );
	}

	bool FillStackLayout::Deserialize (Serializing::Deserializer const& des)
	{
		return des( _name, _origin, _spacing );
	}

/*
=================================================
	Update
=================================================
*/
	void FillStackLayout::Update (const UpdateQueue &queue, const LayoutState &parentState)
	{
		EditState().UpdateAndFillParent( parentState );
		
		queue.AppendDrawable( this, _drawable.get() );
		queue.AppendController( this, _controller.get() );

		float	weight_sum = 0.0f;

		for (usize i = 0; i < _childs.size(); ++i)
		{
			weight_sum += Cast< FillStackLayoutCell >( _childs[i].get() )->Weight();
		}
		
		float2			offset;
		float2 const	size	= State().LocalRect().Size();
		float2 const	scale	= SafeDiv( size - _spacing * float(_childs.size()+1), weight_sum );	// TODO: spacing may be greater then size

		for (usize i = 0; i < _childs.size(); ++i)
		{
			queue.EnqueueLayout( this, _childs[i].get() );

			auto*	cell = Cast< FillStackLayoutCell >( _childs[i].get() );
			RectF	rect;

			switch ( _origin )
			{
				case EStackOrigin::Left :
				{
					offset.x		+= _spacing;
					rect.LeftTop(	   float2{ offset.x, 0.0f });
					offset.x		+= cell->Weight() * scale.x;
					rect.RightBottom(  float2{ offset.x, size.y });
					break;
				}
				case EStackOrigin::Right :
				{
					offset.x		+= _spacing;
					rect.LeftTop(	   float2{ size.x - offset.x, 0.0f });
					offset.x		+= cell->Weight() * scale.x;
					rect.RightBottom(  float2{ size.x - offset.x, size.y });
					break;
				}
				case EStackOrigin::Bottom :
				{
					offset.y		+= _spacing;
					rect.LeftTop(	   float2{ 0.0f, offset.y });
					offset.y		+= cell->Weight() * scale.y;
					rect.RightBottom(  float2{ size.x, offset.y });
					break;
				}
				case EStackOrigin::Top :
				{
					offset.y		+= _spacing;
					rect.LeftTop(	   float2{ 0.0f, size.y - offset.y });
					offset.y		+= cell->Weight() * scale.y;
					rect.RightBottom(  float2{ size.x, size.y - offset.y });
					break;
				}
			}
					
			cell->SetRequiredRegion( rect );
		}
	}
	
/*
=================================================
	GetPreferredSize
=================================================
*/
	RectF FillStackLayout::GetPreferredSize (const RectF &parentRect) const
	{
		return RectF( parentRect.Size() );
	}

/*
=================================================
	AddChild
=================================================
*/
	bool FillStackLayout::AddChild (LayoutPtr &&layout)
	{
		if ( layout->GetTypeId() == typeid(FillStackLayoutCell) )
		{
			_childs.push_back( std::move(layout) );
			return true;
		}
		
		return AddChild( std::move(layout), 1.0f );
	}
	
/*
=================================================
	AddChild
=================================================
*/
	bool FillStackLayout::AddChild (LayoutPtr &&layout, float weight)
	{
		CHECK_ERR( layout->GetTypeId() != typeid(FillStackLayoutCell) );

		_childs.push_back( LayoutPtr( New<FillStackLayoutCell>( std::move(layout), weight )) );
		return true;
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
	constructor
=================================================
*/
	FixedLayout::FixedLayout (StringView name, const RectF &region) :
		DefaultLayout( name ), _region{ region }
	{
		ASSERT( region.IsNormalized() );
	}
	
/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool FixedLayout::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _name, _region );
	}

	bool FixedLayout::Deserialize (Serializing::Deserializer const& des)
	{
		return des( _name, _region );
	}

/*
=================================================
	Update
=================================================
*/
	void FixedLayout::Update (const UpdateQueue &queue, const LayoutState &parentState)
	{
		EditState().Update( parentState, _region );
		
		queue.EnqueueLayout( this, _child.get() );
		
		queue.AppendDrawable( this, _drawable.get() );
		queue.AppendController( this, _controller.get() );
	}
	
/*
=================================================
	GetPreferredSize
=================================================
*/
	RectF FixedLayout::GetPreferredSize (const RectF &) const
	{
		return _region;
	}
//-----------------------------------------------------------------------------
	


/*
=================================================
	constructor
=================================================
*/
	AlignedLayout::AlignedLayout (StringView name, ELayoutAlign align, const float2 &size) :
		DefaultLayout( name ), _align{ align }, _size{ size }
	{
	}
		
/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool AlignedLayout::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _name, _align, _size );
	}

	bool AlignedLayout::Deserialize (Serializing::Deserializer const& des)
	{
		return des( _name, _align, _size );
	}

/*
=================================================
	_CalcAlignedRect
=================================================
*/
	RectF AlignedLayout::_CalcAlignedRect (const RectF &parentRect) const
	{
		RectF const	parent_region { parentRect.Size() };
		RectF		region;

		// horizontal alignment
		if ( AllBits( _align, ELayoutAlign::CenterX ))
		{
			region.left		= parent_region.Center().x - _size.x * 0.5f;
			region.right	= region.left + _size.x;
		}
		else
		if ( AllBits( _align, ELayoutAlign::Left | ELayoutAlign::Right ))
		{
			region.left		= parent_region.left;
			region.right	= parent_region.right;
		}
		else
		if ( AllBits( _align, ELayoutAlign::Left ))
		{
			region.left		= parent_region.left;
			region.right	= region.left + _size.x;
		}
		else
		if ( AllBits( _align, ELayoutAlign::Right ))
		{
			region.right	= parent_region.right;
			region.left		= region.right - _size.x;
		}
		else
		{
			// alignment is undefined
			region.left		= 0.0f;
			region.right	= 0.0f;
		}
		

		// vertical alignment
		if ( AllBits( _align, ELayoutAlign::CenterY ))
		{
			region.top		= parent_region.Center().y - _size.y * 0.5f;
			region.bottom	= region.top + _size.y;
		}
		else
		if ( AllBits( _align, ELayoutAlign::Bottom | ELayoutAlign::Top ))
		{
			region.top		= parent_region.top;
			region.bottom	= parent_region.bottom;
		}
		else
		if ( AllBits( _align, ELayoutAlign::Bottom ))
		{
			region.top		= parent_region.top;
			region.bottom	= region.top + _size.y;
		}
		else
		if ( AllBits( _align, ELayoutAlign::Top ))
		{
			region.bottom	= parent_region.bottom;
			region.top		= region.bottom - _size.y;
		}
		else
		{
			// alignment is undefined
			region.bottom	= 0.0f;
			region.top		= 0.0f;
		}

		return region;
	}

/*
=================================================
	Update
=================================================
*/
	void AlignedLayout::Update (const UpdateQueue &queue, const LayoutState &parentState)
	{
		EditState().Update( parentState, _CalcAlignedRect( parentState.LocalRect() ));

		queue.EnqueueLayout( this, _child.get() );
		
		queue.AppendDrawable( this, _drawable.get() );
		queue.AppendController( this, _controller.get() );
	}
	
/*
=================================================
	GetPreferredSize
=================================================
*/
	RectF AlignedLayout::GetPreferredSize (const RectF &parentRect) const
	{
		return _CalcAlignedRect( parentRect );
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	FixedMarginLayout::FixedMarginLayout (StringView name, const float2 &xMargin, const float2 &yMargin) :
		DefaultLayout( name ),
		_xMargin{Max( float2(0.0f), xMargin )},
		_yMargin{Max( float2(0.0f), yMargin )}
	{
	}
	
/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool FixedMarginLayout::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _name, _xMargin, _yMargin );
	}

	bool FixedMarginLayout::Deserialize (Serializing::Deserializer const& des)
	{
		return des( _name, _xMargin, _yMargin );
	}

/*
=================================================
	Update
=================================================
*/
	void FixedMarginLayout::Update (const UpdateQueue &queue, const LayoutState &parentState)
	{
		if ( not _child )
		{
			EditState().Update( parentState, RectF{} );
			return;
		}

		queue.EnqueueLayout( this, _child.get() );
		queue.EnqueueCallback( this, this, &_AfterUpdate );
		
		queue.AppendDrawable( this, _drawable.get() );
		queue.AppendController( this, _controller.get() );

		EditState().UpdateAndFillParent( parentState );
	}
	
/*
=================================================
	_AfterUpdate
=================================================
*/
	void FixedMarginLayout::_AfterUpdate (ILayout *layout, const UpdateQueue &, const LayoutState &)
	{
		auto*	self	= Cast< FixedMarginLayout >( layout );
		RectF	region	= self->_AddMarginToRect( self->_child->State().LocalRect() );

		self->EditState().Resize( region );
	}

/*
=================================================
	GetPreferredSize
=================================================
*/
	RectF FixedMarginLayout::GetPreferredSize (const RectF &parentRect) const
	{
		if ( _child )
			return _AddMarginToRect( _child->GetPreferredSize( parentRect ));

		return RectF();
	}
	
/*
=================================================
	_AddMarginToRect
=================================================
*/
	RectF FixedMarginLayout::_AddMarginToRect (const RectF &rect) const
	{
		RectF		region{ rect };
		region.left		-= _xMargin[0];
		region.right	+= _xMargin[1];
		region.top		-= _yMargin[0];
		region.bottom	+= _yMargin[1];
		return region;
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	RelativeMarginLayout::RelativeMarginLayout (StringView name, const float2 &xMargin, const float2 &yMargin) :
		DefaultLayout( name ),
		_xMargin{Max( float2(0.0f), xMargin )},
		_yMargin{Max( float2(0.0f), yMargin )}
	{
	}
	
/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool RelativeMarginLayout::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _name, _xMargin, _yMargin );
	}

	bool RelativeMarginLayout::Deserialize (Serializing::Deserializer const& des)
	{
		return des( _name, _xMargin, _yMargin );
	}

/*
=================================================
	Update
=================================================
*/
	void RelativeMarginLayout::Update (const UpdateQueue &queue, const LayoutState &parentState)
	{
		if ( not _child )
		{
			EditState().Update( parentState, RectF() );
			return;
		}

		queue.EnqueueLayout( this, _child.get() );
		queue.EnqueueCallback( this, this, &_AfterUpdate );
		
		queue.AppendDrawable( this, _drawable.get() );
		queue.AppendController( this, _controller.get() );
		
		EditState().UpdateAndFillParent( parentState );
	}
	
/*
=================================================
	_AfterUpdate
=================================================
*/
	void RelativeMarginLayout::_AfterUpdate (ILayout *layout, const UpdateQueue &, const LayoutState &)
	{
		auto*	self	= Cast< RelativeMarginLayout >( layout );
		RectF	region	= self->_AddMarginToRect( self->_child->State().LocalRect() );

		self->EditState().Resize( region );
	}

/*
=================================================
	GetPreferredSize
=================================================
*/
	RectF RelativeMarginLayout::GetPreferredSize (const RectF &parentRect) const
	{
		if ( _child )
			return _AddMarginToRect( _child->GetPreferredSize( parentRect ));

		return RectF();
	}
	
/*
=================================================
	_AddMarginToRect
=================================================
*/
	RectF RelativeMarginLayout::_AddMarginToRect (const RectF &rect) const
	{
		RectF			region{ rect };
		float2 const	size	= region.Size();

		region.left		-= _xMargin[0] * size.x;
		region.right	+= _xMargin[1] * size.x;
		region.top		-= _yMargin[0] * size.y;
		region.bottom	+= _yMargin[1] * size.y;
		return region;
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	FixedPaddingLayout::FixedPaddingLayout (StringView name, const float2 &xPadding, const float2 &yPadding) :
		DefaultLayout( name ),
		_xPadding{Max( float2(0.0f), xPadding )},
		_yPadding{Max( float2(0.0f), yPadding )}
	{
	}

/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool FixedPaddingLayout::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _name, _xPadding, _yPadding );
	}

	bool FixedPaddingLayout::Deserialize (Serializing::Deserializer const& des)
	{
		return des( _name, _xPadding, _yPadding );
	}
	
/*
=================================================
	Update
=================================================
*/
	void FixedPaddingLayout::Update (const UpdateQueue &queue, const LayoutState &parentState)
	{
		// TODO: do not add child, drawable and other if clipped
		queue.EnqueueLayout( this, _child.get() );
		
		queue.AppendDrawable( this, _drawable.get() );
		queue.AppendController( this, _controller.get() );

		EditState().Update( parentState, _AddPaddingToRect( parentState.LocalRect() ));
	}
	
/*
=================================================
	GetPreferredSize
=================================================
*/
	RectF FixedPaddingLayout::GetPreferredSize (const RectF &parentRect) const
	{
		return _AddPaddingToRect( parentRect );
	}
	
/*
=================================================
	_AddPaddingToRect
=================================================
*/
	RectF FixedPaddingLayout::_AddPaddingToRect (const RectF &parentRect) const
	{
		RectF	region{ parentRect.Size() };
		
		region.left		+= _xPadding[0];
		region.right	-= _xPadding[1];
		region.top		+= _yPadding[0];
		region.bottom	-= _yPadding[1];
		
		// if not enough space for child with padding
		if ( Any( region.RightBottom() < region.LeftTop() ))
		{
			region.RightBottom( region.LeftTop() );
		}

		return region;
	}
//-----------------------------------------------------------------------------



/*
=================================================
	constructor
=================================================
*/
	RelativePaddingLayout::RelativePaddingLayout (StringView name, const float2 &xPadding, const float2 &yPadding) :
		DefaultLayout( name ),
		_xPadding{Max( float2(0.0f), xPadding )},
		_yPadding{Max( float2(0.0f), yPadding )}
	{
	}
	
/*
=================================================
	Serialize / Deserialize
=================================================
*/
	bool RelativePaddingLayout::Serialize (Serializing::Serializer &ser) const
	{
		return ser( _name, _xPadding, _yPadding );
	}

	bool RelativePaddingLayout::Deserialize (Serializing::Deserializer const& des)
	{
		return des( _name, _xPadding, _yPadding );
	}

/*
=================================================
	Update
=================================================
*/
	void RelativePaddingLayout::Update (const UpdateQueue &queue, const LayoutState &parentState)
	{
		// TODO: do not add child, drawable and other if clipped
		queue.EnqueueLayout( this, _child.get() );
		
		queue.AppendDrawable( this, _drawable.get() );
		queue.AppendController( this, _controller.get() );
		
		EditState().Update( parentState, _AddPaddingToRect( parentState.LocalRect() ));
	}
	
/*
=================================================
	GetPreferredSize
=================================================
*/
	RectF RelativePaddingLayout::GetPreferredSize (const RectF &parentRect) const
	{
		return _AddPaddingToRect( parentRect );
	}
	
/*
=================================================
	_AddPaddingToRect
=================================================
*/
	RectF RelativePaddingLayout::_AddPaddingToRect (const RectF &parentRect) const
	{
		float2 const	size = parentRect.Size();
		RectF			region{ size };
		
		region.left		+= _xPadding[0] * size.x;
		region.right	-= _xPadding[1] * size.x;
		region.top		+= _yPadding[0] * size.y;
		region.bottom	-= _yPadding[1] * size.y;
		
		// if not enough space for child with padding
		if ( Any( region.RightBottom() < region.LeftTop() ))
		{
			region.RightBottom( region.LeftTop() );
		}

		return region;
	}
//-----------------------------------------------------------------------------


}	// AE::UI
