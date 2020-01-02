// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Common.h"

namespace AE::ECS
{
	struct MsgTag_AddedComponent {};
	struct MsgTag_RemovedComponent {};
	struct MsgTag_ComponentChanged {};


	//
	// Message Builder
	//

	class MessageBuilder
	{
	// types
	private:
		struct MessageKey
		{
			size_t	_value = UMax;

			MessageKey () {}
			MessageKey (ComponentID compId, MsgTagID tagId);

			ND_ bool  operator == (const MessageKey &) const;
		};

		struct MessageKeyHash {
			size_t  operator () (const MessageKey &) const;
		};
		
		struct MessageData
		{
			using Listener_t = Function< void (MessageData &) >;

			Array<EntityID>			entities;
			Array<uint8_t>			components;		// TODO: align
			Array< Listener_t >		listeners;
		};
		
		using MessageMap_t	= HashMap< MessageKey, MessageData, MessageKeyHash >;
		using Pending_t		= Array< MessageData * >;


	// variables
	private:
		MessageMap_t	_msgTypes;
		Pending_t		_pending;


	// methods
	public:

		template <typename Tag>
		void  Add (EntityID id, ComponentID compId);

		template <typename Tag>
		void  Add (EntityID id, ComponentID compId, ArrayView<uint8_t> data);

		template <typename Tag>
		void  Add (EntityID id, ComponentID compId, const Pair<void*, BytesU> &data);

		template <typename Tag, typename Comp>
		void  Add (EntityID id, const Comp& comp);
		
		template <typename Tag>
		void  AddMulti (ComponentID compId, ArrayView<EntityID> ids, ArrayView<uint8_t> data);
		
		template <typename Tag>
		void  AddMulti (ComponentID compId, ArrayView<EntityID> ids);

		template <typename Comp, typename Tag, typename Fn>
		bool  AddListener (Fn &&fn);

		template <typename Tag>
		ND_ bool  HasListener (ComponentID compId) const;

		void  Process ();
	};

	
			
	
	inline MessageBuilder::MessageKey::MessageKey (ComponentID compId, MsgTagID tagId) :
		_value{ (size_t(compId.value) << 16) | size_t(tagId.value) }
	{}
	
	inline bool  MessageBuilder::MessageKey::operator == (const MessageKey &rhs) const
	{
		return _value == rhs._value;
	}
//-----------------------------------------------------------------------------


	inline size_t  MessageBuilder::MessageKeyHash::operator () (const MessageKey &x) const
	{
		return x._value;
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	Add
=================================================
*/
	template <typename Tag>
	inline void  MessageBuilder::Add (EntityID id, ComponentID compId)
	{
		MessageKey const	key  { compId, MsgTagTypeInfo<Tag>::id };
		auto				iter = _msgTypes.find( key );
		
		// no listener to process this message
		if ( iter == _msgTypes.end() )
			return;
		
		auto&	msg = iter->second;

		if ( msg.entities.empty() )
			_pending.push_back( &msg );
		
		ASSERT( msg.components.empty() );

		msg.entities.push_back( id );
	}

/*
=================================================
	Add
=================================================
*/
	template <typename Tag>
	inline void  MessageBuilder::Add (EntityID id, ComponentID compId, ArrayView<uint8_t> comp)
	{
		MessageKey const	key  { compId, MsgTagTypeInfo<Tag>::id };
		auto				iter = _msgTypes.find( key );
		
		// no listener to process this message
		if ( iter == _msgTypes.end() )
			return;

		auto&	msg = iter->second;

		if ( msg.entities.empty() )
			_pending.push_back( &msg );
		
		ASSERT( msg.entities.empty() or not msg.components.empty() );

		const size_t	size = comp.size();
		msg.components.resize( (msg.entities.size() + 1) * size );
		std::memcpy( OUT msg.components.data() + BytesU{msg.entities.size() * size}, comp.data(), size );

		msg.entities.push_back( id );
	}

	template <typename Tag, typename Comp>
	inline void  MessageBuilder::Add (EntityID id, const Comp& comp)
	{
		return Add<Tag>( id, ComponentTypeInfo<Comp>::id, ArrayView<uint8_t>{ Cast<uint8_t>(&comp), sizeof(comp) });
	}
	
	template <typename Tag>
	inline void  MessageBuilder::Add (EntityID id, ComponentID compId, const Pair<void*, BytesU> &data)
	{
		return Add<Tag>( id, compId, ArrayView<uint8_t>{ Cast<uint8_t>(data.first), size_t(data.second) });
	}

/*
=================================================
	AddListener
=================================================
*/
	template <typename Comp, typename Tag, typename Fn>
	inline bool  MessageBuilder::AddListener (Fn &&fn)
	{
		using FI = FunctionInfo< Fn >;

		MessageKey	key{ ComponentTypeInfo<Comp>::id, MsgTagTypeInfo<Tag>::id };
		auto&		msg = _msgTypes[ key ];

		if constexpr( FI::args::Count == 1 )
		{
			STATIC_ASSERT( IsSameTypes<typename FI::args::template Get<0>, ArrayView<EntityID>> );
			//STATIC_ASSERT( not IsSameTypes< Tag, MsgTag_RemovedComponent >);
			
			msg.listeners.push_back(
				[fn = std::forward<Fn>(fn)] (const MessageData &msg)
				{
					fn( ArrayView<EntityID>{ msg.entities });
				});
			return true;
		}

		if constexpr( FI::args::Count == 2 )
		{
			STATIC_ASSERT( IsSameTypes<typename FI::args::template Get<0>, ArrayView<EntityID>> );
			STATIC_ASSERT( IsSameTypes<typename FI::args::template Get<1>, ArrayView<Comp>> );
			
			msg.listeners.push_back(
				[fn = std::forward<Fn>(fn)] (const MessageData &msg)
				{
					fn( ArrayView<EntityID>{ msg.entities },
					    ArrayView<Comp>{ Cast<Comp>(msg.components.data()), msg.entities.size() });
				});
			return true;
		}
	}
	
/*
=================================================
	AddMulti
=================================================
*/
	template <typename Tag>
	inline void  MessageBuilder::AddMulti (ComponentID compId, ArrayView<EntityID> ids, ArrayView<uint8_t> compData)
	{
		MessageKey const	key  { compId, MsgTagTypeInfo<Tag>::id };
		auto				iter = _msgTypes.find( key );
		
		// no listener to process this message
		if ( iter == _msgTypes.end() )
			return;

		auto&	msg = iter->second;

		if ( msg.entities.empty() )
			_pending.push_back( &msg );
		
		ASSERT( msg.entities.empty() or not msg.components.empty() );

		const size_t	size = compData.size();
		msg.components.resize( (msg.entities.size() + 1) * size );
		std::memcpy( OUT msg.components.data() + BytesU{msg.entities.size() * size}, compData.data(), size );

		msg.entities.insert( msg.entities.end(), ids.begin(), ids.end() );
	}
	
/*
=================================================
	AddMulti
=================================================
*/
	template <typename Tag>
	inline void  MessageBuilder::AddMulti (ComponentID compId, ArrayView<EntityID> ids)
	{
		MessageKey const	key  { compId, MsgTagTypeInfo<Tag>::id };
		auto				iter = _msgTypes.find( key );
		
		// no listener to process this message
		if ( iter == _msgTypes.end() )
			return;
		
		auto&	msg = iter->second;

		if ( msg.entities.empty() )
			_pending.push_back( &msg );
		
		ASSERT( msg.components.empty() );
		
		msg.entities.insert( msg.entities.end(), ids.begin(), ids.end() );
	}

/*
=================================================
	HasListener
=================================================
*/
	template <typename Tag>
	inline bool  MessageBuilder::HasListener (ComponentID compId) const
	{
		MessageKey	key{ compId, MsgTagTypeInfo<Tag>::id };
		auto		iter = _msgTypes.find( key );
		
		return iter != _msgTypes.end();
	}

/*
=================================================
	Process
=================================================
*/
	inline void  MessageBuilder::Process ()
	{
		for (auto* msg : _pending)
		{
			for (auto& ml : msg->listeners)
			{
				ml( *msg );
			}

			msg->components.clear();
			msg->entities.clear();
		}

		_pending.clear();
	}

}	// AE::ECS
