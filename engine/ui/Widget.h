// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ui/LayoutClasses.h"
#include "stl/Memory/LinearAllocator.h"

namespace AE::UI
{
	using WidgetPtr	= RC< class Widget >;



	//
	// Widget
	//

	class Widget : public EnableRC<Widget>
	{
	// types
	private:
		using Allocator_t	= LinearAllocator<>;


	// variables
	private:
		WidgetLayout		_layout;
		Allocator_t			_layoutAllocator;

		//WidgetPtr			_parent;
		//ILayout *			_parentLayout	= null;


	// methods
	public:
		Widget ();

		void Update (INOUT ILayout::UpdateQueue &);

		//void SetParent (const WidgetPtr &, ILayout* = null);

		ND_ bool			IsOpaque ()		const	{ return true; }	// TODO
		ND_ ILayout*		GetLayout ()			{ return &_layout; }
		//ND_ ILayout*		GetParentLayout ()		{ return _parentLayout; }

		ND_ Allocator_t&	GetLayoutAllocator ()	{ return _layoutAllocator; }
	};


}	// AE::UI
