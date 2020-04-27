// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ui/Widget.h"
#include "ui/Controller.h"
#include "ui/Drawable.h"

namespace AE::UI
{

/*
=================================================
	constructor
=================================================
*/
	Widget::Widget () :
		_layout{ "root" }
	{
	}
	
/*
=================================================
	Update
=================================================
*/
	void  Widget::Update (INOUT ILayout::UpdateQueue &queue)
	{
		queue.EnqueueLayout( null, &_layout );
		queue.FlushQueue();
	}

/*
=================================================
	SetParent
=================================================
*
	void Widget::SetParent (const WidgetPtr &parent, ILayout* layout)
	{
		_parent			= parent;
		_parentLayout	= _parent ? (layout ? layout : _parent->GetLayout()) : null; 
	}
	*/

}	// AE::UI
