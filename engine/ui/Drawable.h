// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "serializing/ISerializable.h"
#include "ui/Common.h"

namespace AE::UI
{
	struct LayoutState;
	struct UIRendererData;


	//
	// Drawable interface
	//
	class IDrawable : public Serializing::ISerializable
	{
	// interface
	public:
		virtual ~IDrawable () {}

		virtual void Draw (const UIRendererData &, const LayoutState &, float dt) = 0;

		virtual bool GetPreferredSize (const RectF &maxRect, OUT RectF &result) const = 0;
	};

}	// AE::UI
