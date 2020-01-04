// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "platform/Public/KeyEnums.h"

namespace AE::App
{

	//
	// Input Event Queue
	//

	class InputEventQueue : public Noncopyable
	{
	// types
	public:
		using Milliseconds	= std::chrono::duration<uint, std::milli>;

		struct EventBase
		{
			uint			eventType	= UMax;
			Milliseconds	timestamp;
		};

		struct KeyEvent : EventBase
		{
			EKey			key;
			EKeyState		state;
		};

		struct TouchEvent : EventBase
		{
			enum class EAction : uint8_t {
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
				uint8_t		id;
			};
			static constexpr uint	MaxTouches	= 4;

			Touch		data [MaxTouches];
			EAction		action;
			uint8_t		count;
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
		template <typename T>
		struct _GetTypeSize {
			static constexpr size_t	value = sizeof(T);
		};
		
		template <typename T>
		struct _GetTypeAlign {
			static constexpr size_t	value = alignof(T);
		};

		static constexpr size_t	_MaxStorageSize		= MsgTypes_t::ForEach_Max< _GetTypeSize >();
		static constexpr size_t	_MaxStorageAlign	= MsgTypes_t::ForEach_Max< _GetTypeAlign >();
		
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


	// methods
	public:
		InputEventQueue () {}

		ND_ bool  Pop (OUT MsgStorage &msg)
		{
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
		void Push (T &msg)
		{
			STATIC_ASSERT( MsgTypes_t::HasType<T> );
			ASSERT( _queue.size() < 1024 );

			msg.eventType = MsgTypes_t::Index<T>;

			std::memcpy( OUT &_queue.emplace_back(), &msg, sizeof(msg) );
		}
	};


}	// AE::App
