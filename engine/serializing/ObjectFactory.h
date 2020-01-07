// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "serializing/ISerializable.h"
#include "serializing/Serializer.h"
#include "serializing/Deserializer.h"

namespace AE::Serializing
{

	//
	// Object Factory
	//

	class ObjectFactory
	{
	// types
	private:
		using Serialize_t	= bool (*) (Serializer &, const void *);
		using Deserialize_t	= bool (*) (const Deserializer &, OUT void *, bool create);

		struct ObjInfo
		{
			Serialize_t		serialize	= null;
			Deserialize_t	deserialize	= null;
		};

		using ObjectMap_t	= HashMap< SerializedID, ObjInfo >;
		using ObjectTypes_t	= HashMap< std::type_index, Pair<const SerializedID, ObjInfo>* >;
		using HashToObj_t	= HashMap< uint, Pair<const SerializedID, ObjInfo>* >;


	// variables
	private:
		SharedMutex				_guard;
		ObjectMap_t				_objects;
		ObjectTypes_t			_objectTypes;

		#if not AE_OPTIMIZE_IDS
			HashToObj_t			_hashToObj;
		#endif

		DEBUG_ONLY(
			NamedID_HashCollisionCheck	_hashCollisionCheck;
		)


	// methods
	public:
		ObjectFactory () {}

		template <typename T>
		bool Register (const SerializedID &id, Serialize_t ser, Deserialize_t deser);
		
		template <typename T>
		bool Register (const SerializedID &id);

		template <typename T>
		bool Serialize (Serializer &, const T& obj);

		template <typename T>
		bool Deserialize (const Deserializer &, INOUT T& obj);
		bool Deserialize (const Deserializer &, INOUT void* obj);
	};

	
/*
=================================================
	Register
=================================================
*/
	template <typename T>
	inline bool  ObjectFactory::Register (const SerializedID &id, Serialize_t ser, Deserialize_t deser)
	{
		STATIC_ASSERT( not IsPOD<T> );
		EXLOCK( _guard );

		auto[iter, inserted] = _objects.insert({ id, ObjInfo{ser, deser} });
		CHECK_ERR( inserted );

		CHECK_ERR( _objectTypes.insert({ typeid(T), iter.operator->() }).second );
		
		#if not AE_OPTIMIZE_IDS
			_hashToObj.insert({ uint(size_t(id.GetHash())), iter.operator->() });
		#endif

		DEBUG_ONLY( _hashCollisionCheck.Add( id ));
		return true;
	}
	
	template <typename T>
	inline bool  ObjectFactory::Register (const SerializedID &id)
	{
		STATIC_ASSERT( IsBaseOf< ISerializable, T > );
		return Register<T>( id,
							[] (Serializer &ser, const void *ptr) {
								return Cast<ISerializable>(ptr)->Serialize( ser );
							},
							[] (const Deserializer &des, OUT void *ptr, bool create) {
								if ( create ) PlacementNew<T>(ptr);
								return Cast<ISerializable>(ptr)->Deserialize( des );
							}
						  );
	}

/*
=================================================
	Serialize
=================================================
*/
	template <typename T>
	inline bool  ObjectFactory::Serialize (Serializer &ser, const T& obj)
	{
		STATIC_ASSERT( not IsPOD<T> );
		SHAREDLOCK( _guard );

		auto	iter = _objectTypes.find( typeid(T) );
		CHECK_ERR( iter != _objectTypes.end() );

		CHECK_ERR( ser.stream->Write( uint(size_t(iter->second->first.GetHash())) ) and
				   iter->second->second.serialize( ser, &obj ));
		return true;
	}

/*
=================================================
	Deserialize
=================================================
*/
	template <typename T>
	inline bool  ObjectFactory::Deserialize (const Deserializer &deser, INOUT T& obj)
	{
		STATIC_ASSERT( not IsPOD<T> );
		SHAREDLOCK( _guard );
		
		auto	iter = _objectTypes.find( typeid(T) );
		CHECK_ERR( iter != _objectTypes.end() );

		uint	id;
		CHECK_ERR( deser.stream->Read( OUT id )							and
				   id == uint(size_t(iter->second->first.GetHash()))	and
				   iter->second->second.deserialize( deser, INOUT &obj, false ));
		return true;
	}
	
/*
=================================================
	Deserialize
=================================================
*/
	inline bool  ObjectFactory::Deserialize (const Deserializer &deser, INOUT void* obj)
	{
		SHAREDLOCK( _guard );
		
		uint	id = 0;
		CHECK_ERR( deser.stream->Read( OUT id ));

		ObjInfo*	info = null;

		#if AE_OPTIMIZE_IDS
			auto	iter = _objects.find( SerializedID{id} );
			CHECK_ERR( iter != _objects.end() );
			info = &iter->second;
		#else
			auto	iter = _hashToObj.find( id );
			CHECK_ERR( iter != _hashToObj.end() );
			info = &iter->second->second;
		#endif

		CHECK_ERR( info->deserialize( deser, INOUT obj, true ));
		return true;
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	Serializer::_SerializeObj
=================================================
*/
	template <typename T>
	inline bool  Serializer::_SerializeObj (const T &obj)
	{
		if ( factory )
			return factory->Serialize( *this, obj );
		
		if constexpr( IsBaseOf< ISerializable, T >)
			return obj.Serialize( *this );
		else
		{
			ASSERT( !"unknown type" );
			return false;
		}
	}
	
/*
=================================================
	Deserializer::_DeserializeObj
=================================================
*/
	template <typename T>
	inline bool  Deserializer::_DeserializeObj (INOUT T &obj) const
	{
		if ( factory )
			return factory->Deserialize( *this, INOUT obj );
		
		if constexpr( IsBaseOf< ISerializable, T >)
			return obj.Deserialize( *this );
		else
		{
			ASSERT( !"unknown type" );
			return false;
		}
	}
	
	inline bool  Deserializer::operator () (INOUT void *obj) const
	{
		if ( factory )
			return factory->Deserialize( *this, INOUT obj );
		
		ASSERT( !"unknown type" );
		return false;
	}


}	// AE::Serializing
