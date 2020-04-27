// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ui/Layout.h"

namespace AE::UI
{

	//
	// Default Layout
	//
	class DefaultLayout : public ILayout
	{
	private:
		using Name_t	= FixedString<32>;

	// variables
	protected:
		LayoutPtr		_child;
		DrawablePtr		_drawable;
		ControllerPtr	_controller;
		Name_t			_name;

	// methods
	protected:
		DefaultLayout ();
		explicit DefaultLayout (StringView name);
		~DefaultLayout ();
		
	public:
		StringView Name () const override		{ return _name; }

		bool AddChild (LayoutPtr &&) override;
		bool SetDrawable (DrawablePtr &&) override;
		bool SetController (ControllerPtr &&) override;
	};



	//
	// Default Array Layout
	//
	class DefaultArrayLayout : public ILayout
	{
	private:
		using Name_t	= FixedString<32>;

	// variables
	protected:
		Array<LayoutPtr>	_childs;
		DrawablePtr			_drawable;
		ControllerPtr		_controller;
		Name_t				_name;

	// methods
	protected:
		DefaultArrayLayout ();
		explicit DefaultArrayLayout (StringView name);
		~DefaultArrayLayout ();
		
	public:
		StringView Name () const override		{ return _name; }

		bool AddChild (LayoutPtr &&) override;
		bool SetDrawable (DrawablePtr &&) override;
		bool SetController (ControllerPtr &&) override;
	};



	//
	// Widget Layout (TODO)
	//
	class WidgetLayout final : public DefaultLayout
	{
	// methods
	public:
		WidgetLayout () {}
		explicit WidgetLayout (StringView name);
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (const UpdateQueue &, const LayoutState &) override;
		
		//RectF GetPreferredSize (const RectF &) const override;

		std::type_index  GetTypeId () const override	{ return typeid( *this ); }
	};



	//
	// Auto Size Layout
	//
	class AutoSizeLayout : public DefaultArrayLayout
	{
	// methods
	public:
		AutoSizeLayout () {}
		explicit AutoSizeLayout (StringView name);
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (const UpdateQueue &, const LayoutState &) override;

		RectF GetPreferredSize (const RectF &) const override;

		std::type_index  GetTypeId () const override	{ return typeid( *this ); }

	protected:
		static void _AfterUpdate (ILayout*, const UpdateQueue &, const LayoutState &);
	};



	//
	// Stack Layout
	//
	class StackLayout final : public DefaultArrayLayout
	{
	// variables
	private:
		EStackOrigin	_origin	= Default;

	// methods
	public:
		StackLayout () {}
		StackLayout (StringView name, EStackOrigin origin);
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (const UpdateQueue &, const LayoutState &) override;
		
		RectF GetPreferredSize (const RectF &) const override;

		bool AddChild (LayoutPtr &&) override;

		std::type_index  GetTypeId ()	const override	{ return typeid( *this ); }
		
		EDirection		Direction ()	const			;//{ return EDirection::From( _origin ); }
		EStackOrigin	Origin ()		const			{ return _origin; }
	};



	//
	// Fill Stack Layout
	//
	class FillStackLayout final : public DefaultArrayLayout
	{
	// variables
	private:
		EStackOrigin	_origin		= Default;
		float			_spacing	= 0.0f;

	// methods
	public:
		FillStackLayout () {}
		FillStackLayout (StringView name, EStackOrigin origin, float spacing = 0.0f);
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (const UpdateQueue &, const LayoutState &) override;
		
		RectF GetPreferredSize (const RectF &) const override;

		bool AddChild (LayoutPtr &&) override;
		bool AddChild (LayoutPtr &&, float weight);

		std::type_index  GetTypeId () const override	{ return typeid( *this ); }

		void SetSpacing (float value)					{ _spacing = value; }

		float			Spacing ()		const			{ return _spacing; }
		EDirection		Direction ()	const			;//{ return EDirection::From( _origin ); }
		EStackOrigin	Origin ()		const			{ return _origin; }
	};



	//
	// Fixed Layout
	//
	class FixedLayout final : public DefaultLayout
	{
	// variables
	private:
		RectF		_region;

	// methods
	public:
		FixedLayout () {}
		FixedLayout (StringView name, const RectF &region);
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (const UpdateQueue &, const LayoutState &) override;
		
		RectF GetPreferredSize (const RectF &) const override;

		std::type_index GetTypeId () const override					{ return typeid( *this ); }

		ND_ RectF const&	GetRegion ()					const	{ return _region; }
			void			SetRegion (const RectF &value)			{ _region = value; }
			void			Move (const float2 &delta)				{ _region += delta; }
	};



	//
	// Aligned Layout
	//
	class AlignedLayout final : public DefaultLayout
	{
	// variables
	private:
		ELayoutAlign	_align	= Default;
		float2			_size;

	// methods
	public:
		AlignedLayout () {}
		AlignedLayout (StringView name, ELayoutAlign align, const float2 &size);
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (const UpdateQueue &, const LayoutState &) override;
		
		RectF GetPreferredSize (const RectF &) const override;

		std::type_index  GetTypeId () const override	{ return typeid( *this ); }

		void SetSize (const float2 &value)				{ _size = value; }
		void SetAlign (ELayoutAlign value)				{ _align = value; }

	private:
		RectF _CalcAlignedRect (const RectF &) const;
	};



	//
	// Scrollable Layout
	//
	/*class ScrollableLayout final : public DefaultArrayLayout
	{
	// variables
	private:
		EDirection		_scroll		= Default;

	// methods
	public:
		ScrollableLayout (StringView name, EDirection scroll);

		void Update (const UpdateQueue &, const LayoutState &) override;

		bool AddChild (LayoutPtr &&) override;
		std::type_index GetTypeId () const override		{ return typeid( *this ); }
	};*/



	//
	// Fixed Margin Layout
	//
	class FixedMarginLayout final : public DefaultLayout
	{
	// variables
	private:
		float2		_xMargin;	// in pixels
		float2		_yMargin;

	// methods
	public:
		FixedMarginLayout () {}
		FixedMarginLayout (StringView name, const float2 &xMargin, const float2 &yMargin);
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (const UpdateQueue &, const LayoutState &) override;
		
		RectF GetPreferredSize (const RectF &) const override;

		std::type_index  GetTypeId () const override	{ return typeid( *this ); }
		
		void SetMarginX (float left, float right)		{ _xMargin = float2{left, right}; }
		void SetMarginY (float bottom, float top)		{ _yMargin = float2{bottom, top}; }

	private:
		RectF _AddMarginToRect (const RectF &) const;

		static void _AfterUpdate (ILayout*, const UpdateQueue &, const LayoutState &);
	};



	//
	// Relative Margin Layout
	//
	class RelativeMarginLayout final : public DefaultLayout
	{
	// variables
	private:
		float2		_xMargin;	// fractional 0..1 of parent size
		float2		_yMargin;

	// methods
	public:
		RelativeMarginLayout () {}
		RelativeMarginLayout (StringView name, const float2 &xMargin, const float2 &yMargin);
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (const UpdateQueue &, const LayoutState &) override;
		
		RectF GetPreferredSize (const RectF &) const override;

		std::type_index  GetTypeId () const override	{ return typeid( *this ); }
		
		void SetMarginX (float left, float right)		{ _xMargin = float2{left, right}; }
		void SetMarginY (float bottom, float top)		{ _yMargin = float2{bottom, top}; }

	private:
		RectF _AddMarginToRect (const RectF &) const;

		static void _AfterUpdate (ILayout*, const UpdateQueue &, const LayoutState &);
	};



	//
	// Fixed Padding Layout
	//
	class FixedPaddingLayout final : public DefaultLayout
	{
	// variables
	private:
		float2		_xPadding;		// in pixels
		float2		_yPadding;

	// methods
	public:
		FixedPaddingLayout () {}
		FixedPaddingLayout (StringView name, const float2 &xPadding, const float2 &yPadding);
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (const UpdateQueue &, const LayoutState &) override;
		
		RectF GetPreferredSize (const RectF &) const override;

		std::type_index  GetTypeId () const override	{ return typeid( *this ); }
		
		void SetPaddingX (float left, float right)		{ _xPadding = float2{left, right}; }
		void SetPaddingY (float bottom, float top)		{ _yPadding = float2{bottom, top}; }

	private:
		RectF _AddPaddingToRect (const RectF &) const;
	};



	//
	// Relative Padding Layout
	//
	class RelativePaddingLayout final : public DefaultLayout
	{
	// variables
	private:
		float2		_xPadding;		// fractional 0..1 of parent size
		float2		_yPadding;

	// methods
	public:
		RelativePaddingLayout () {}
		RelativePaddingLayout (StringView name, const float2 &xPadding, const float2 &yPadding);
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (const UpdateQueue &, const LayoutState &) override;
		
		RectF GetPreferredSize (const RectF &) const override;

		std::type_index  GetTypeId () const override	{ return typeid( *this ); }
		
		void SetPaddingX (float left, float right)		{ _xPadding = float2{left, right}; }
		void SetPaddingY (float bottom, float top)		{ _yPadding = float2{bottom, top}; }

	private:
		RectF _AddPaddingToRect (const RectF &) const;
	};


}	// AE::UI
