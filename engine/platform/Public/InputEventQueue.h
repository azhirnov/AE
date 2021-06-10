// Copyright (c) 2018-2021,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "platform/Public/KeyEnums.h"
#include "platform/Public/Clock.h"

namespace AE::App
{

	//
	// Input Event Queue
	//

	class InputEventQueue : public Noncopyable
	{
	// types
	public:
		struct EventBase
		{
			uint			eventType	= UMax;
			nanoseconds		timestamp;
		};

		struct KeyEvent : EventBase
		{
			EKey			key;
			EKeyState		state;
		};

		struct TouchEvent : EventBase
		{
			enum class EAction : ubyte {
				Down,
				Up,
				Move,
				Cancel,
				Outside,
				Unknown
			};
			struct Touch {
				float2		pos;
				float		pressure;
				ubyte		id;
			};
			static constexpr uint	MaxTouches	= 4;

			Touch		data [MaxTouches];
			EAction		action;
			ubyte		count;
		};

		struct MouseEvent : EventBase
		{
			int2		pos;	// TODO: use float2 ?
		};
		

		using MsgTypes_t = TypeList< KeyEvent, TouchEvent, MouseEvent >;

		struct EventType
		{
			static constexpr uint	Key		= MsgTypes_t::Index< KeyEvent >;
			static constexpr uint	Touch	= MsgTypes_t::Index< TouchEvent >;
			static constexpr uint	Mouse	= MsgTypes_t::Index< MouseEvent >;
		};

		// TODO: joystic, VR controller, gamepad, sensors ?


	protected:
		static constexpr usize	_MaxStorageSize		= MsgTypes_t::ForEach_Max< TypeListUtils::GetTypeSize >();
		static constexpr usize	_MaxStorageAlign	= MsgTypes_t::ForEach_Max< TypeListUtils::GetTypeAlign >();
		
		using _MsgStorage_t = std::aligned_storage_t< _MaxStorageSize, _MaxStorageAlign >;

	public:
		union MsgStorage
		{
			_MsgStorage_t	_placeholder;
			EventBase		content;

			MsgStorage () : content{} {}
		};


	// variables
	protected:
		Array< MsgStorage >		_queue;		// TODO: use lock-free queue
		Threading::Mutex		_guard;


	// methods
	public:
		InputEventQueue () {}

		ND_ bool  Pop (OUT MsgStorage &msg)
		{
			EXLOCK( _guard );

			if ( _queue.size() )
			{
				std::memcpy( OUT &msg, &_queue.front(), sizeof(MsgStorage) );
				_queue.erase( _queue.begin() );
				return true;
			}
			return false;
		}
	};



	//
	// Input Event Queue Writer
	//

	class InputEventQueueWriter final : public InputEventQueue
	{
	// methods
	public:
		template <typename T>
		void  Push (T &msg)
		{
			STATIC_ASSERT( MsgTypes_t::HasType<T> );
	
			EXLOCK( _guard );
			ASSERT( _queue.size() < 1024 );

			msg.eventType = MsgTypes_t::Index<T>;

			std::memcpy( OUT &_queue.emplace_back(), &msg, sizeof(msg) );
		}
	};


}	// AE::App
