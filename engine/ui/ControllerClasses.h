// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ui/Controller.h"
#include "ui/Layout.h"

namespace AE::UI
{

	//
	// Button Controller
	//
	class ButtonController final : public IController
	{
	// variables
	private:
		UIActionID	_onTap;
		UIActionID	_onDoubleTap;


	// methods
	public:
		ButtonController () {}

		explicit ButtonController (UIActionID onTap, UIActionID onDoubleTap = Default) :
			_onTap{onTap}, _onDoubleTap{onDoubleTap} {}
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (InputState const &, UIActionMap const&, ILayout *) override;
	};
	


	//
	// Selection Controller
	//
	class SelectionController final : public IController
	{
	// variables
	private:
		UIActionID	_onSelectionChanged;
		bool		_selected	= false;


	// methods
	public:
		SelectionController () {}

		explicit SelectionController (UIActionID onSelectionChanged, bool selected = false) :
			_onSelectionChanged{onSelectionChanged}, _selected{selected} {}
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (InputState const &, UIActionMap const&, ILayout *) override;
	};
	


	//
	// Draggable Controller
	//
	class DraggableController final : public IController
	{
	// variables
	private:
		bool		_isDragging		= false;


	// methods
	public:
		DraggableController () {}
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (InputState const &, UIActionMap const&, ILayout *) override;
	};
	


	//
	// Resizable Controller
	//
	class ResizableController final : public IController
	{
	// variables
	private:
		ELayoutAlign	_direction	= Default;


	// methods
	public:
		ResizableController () {}
		
		bool Serialize (Serializing::Serializer &) const override;
		bool Deserialize (Serializing::Deserializer const&) override;

		void Update (InputState const &, UIActionMap const&, ILayout *) override;
	};

	

	inline LayoutState&  IController::EditLayoutState (ILayout *layout)
	{
		return layout->EditState();
	}

}	// AE::UI
