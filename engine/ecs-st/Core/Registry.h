// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Core/Archetype.h"
#include "ecs-st/Core/EntityPool.h"
#include "ecs-st/Core/MessageBuilder.h"

namespace AE::ECS
{
	class Registry;


	//
	// Component Access Type
	//

	template <typename T>
	struct WriteAccess
	{
		STATIC_ASSERT( not IsEmpty<T> );
	private:
		T*  _elements;

	public:
		explicit WriteAccess (T* elems) : _elements{ elems } { ASSERT( _elements ); }

		ND_ T&  operator [] (size_t i)	const	{ return _elements[i]; }
	};


	template <typename T>
	struct ReadAccess
	{
		STATIC_ASSERT( not IsEmpty<T> );
	private:
		T const*  _elements;

	public:
		explicit ReadAccess (T const* elems) : _elements{ elems } { ASSERT( _elements ); }

		ND_ T const&  operator [] (size_t i)	const	{ return _elements[i]; }
	};
	

	template <typename T>
	struct OptionalWriteAccess
	{
		STATIC_ASSERT( not IsEmpty<T> );
	private:
		T*  _elements;			// can be null

	public:
		explicit OptionalWriteAccess (T* elems) : _elements{ elems } {}

		ND_ T&  operator [] (size_t i)	const	{ ASSERT( _elements != null ); return _elements[i]; }
		ND_ explicit operator bool ()	const	{ return _elements != null; }
	};


	template <typename T>
	struct OptionalReadAccess
	{
		STATIC_ASSERT( not IsEmpty<T> );
	private:
		T const*  _elements;	// can be null

	public:
		explicit OptionalReadAccess (T const* elems) : _elements{ elems } {}

		ND_ T const&  operator [] (size_t i)	const	{ ASSERT( _elements != null ); return _elements[i]; }
		ND_ explicit operator bool ()			const	{ return _elements != null; }
	};
	

	template <typename ...Types>
	struct Subtractive {};
	
	template <typename ...Types>
	struct Require {};
	
	template <typename ...Types>
	struct RequireAny {};



	//
	// System Event helpers
	//

	template <typename T>
	struct AfterEvent {};

	template <typename T>
	struct BeforeEvent {};


	//
	// Query
	//

	enum class EntityQuery : uint
	{};

	struct EntityQueryDesc
	{
	};


	//
	// System
	//

	struct System
	{
		friend class Registry;

	private:
		EntityQuery		_queryId;
		Registry &		_owner;

	protected:
		System (Registry &owner) : _owner{owner} {}

		ND_ Registry&  Owner ()		{ return _owner; }
	};


	//
	// Registry
	//

	class Registry final : public std::enable_shared_from_this< Registry >
	{
	// types
	private:
		using ArchetypeStorages_t	= Array< UniquePtr<ArchetypeStorage> >;
		using ArchetypeMap_t		= HashMap< Archetype, ArchetypeStorages_t >;
		using Index_t				= ArchetypeStorage::Index_t;

		class SingleCompWrap final : public Noncopyable
		{
		public:
			void*	data			= null;
			void(*	deleter)(void*)	= null;

			SingleCompWrap () {}

			~SingleCompWrap ()
			{
				if ( data and deleter )
					deleter( data );
			}
		};

		using SingleCompMap_t		= HashMap< TypeId, SingleCompWrap >;
		using SCAllocator_t			= UntypedAllocator;

		using EventListener_t		= Function< void () >;
		using EventListeners_t		= HashMultiMap< TypeId, EventListener_t >;
		using EventQueue_t			= Array< Function< void () >>;

		struct EntityQueryData
		{
			EntityQueryDesc		desc;
			Array<ArchetypeID>	archetypes;
		};

		using Queries_t			= HashMap< EntityQuery, EntityQueryData >;

		using SystemMap_t		= HashMap< TypeId, UniquePtr<System> >;


	// variables
	private:
		// entity + components
		EntityPool			_entities;
		ArchetypeMap_t		_archetypes;		// don't erase elements!
		MessageBuilder		_messages;

		// single components
		SingleCompMap_t		_singleComponents;
		//SCAllocator_t		_scAllocator;		// TODO

		Queries_t			_queries;
		SystemMap_t			_systemMap;

		EventListeners_t	_eventListeners;
		EventQueue_t		_eventQueue;
		EventQueue_t		_pendingEvents;

		DataRaceCheck		_drCheck;


	// methods
	public:
		Registry ();
		~Registry ();


		// entity
			template <typename ...Components>
			EntityID	CreateEntity ();
		ND_ EntityID	CreateEntity ();
			bool		DestroyEntity (EntityID id);
			void		DestroyAllEntities ();

		ND_ Archetype const*  GetArchetype (EntityID id);

		// component
			template <typename T>
			T&  AssignComponent (EntityID id);
		
			template <typename T>
			bool  RemoveComponent (EntityID id);

			template <typename T>
		ND_ T*  GetComponent (EntityID id);


		// single component
			template <typename T>
			T&  AssignSingleComponent ();
		
			template <typename T>
			bool  RemoveSingleComponent ();

			template <typename T>
		ND_ T*  GetSingleComponent ();

			void DestroyAllSingleComponents ();


		// system
		ND_	EntityQuery  CreateQuery (const EntityQueryDesc &);
		
		template <typename Fn>
		ND_	EntityQuery  CreateQuery ();

			template <typename SystemType>
			void  Run (SystemType &&);

			template <typename Fn>
			void  Enque (Fn &&fn);
			
			template <typename Class, typename ...Args>
			void  Enque (Class obj, void (Class::*)(Args&&...));

			template <typename Ev>
			void  EnqueEvent ();

			void  Process ();
			
			template <typename SystemType, typename Ev>
			void  AddSystem ();

			template <typename Ev, typename Fn>
			void  AddEventListener (Fn &&fn);


		// entity events
			//void OnEntityDestroyed (Function<void (ArrayView<EntityID>)> fn);

			template <typename Comp, typename Tag>
			void OnComponentChanged (Function<void (EntityID, Comp&)> fn);


	private:
			template <typename Ev>
			void  _RunEvent ();

			bool _RemoveEntity (EntityID id);
			void _AddEntity (const Archetype &arch, EntityID id, OUT ArchetypeStorage* &storage, OUT Index_t &index);
			void _AddEntity (const Archetype &arch, EntityID id);

			template <typename ArgsList, size_t I = 0>
		ND_ static bool  _IsArchetypeSupported (const Archetype &arch);

			template <typename ...Args>
		ND_ static Tuple<size_t, Args...>  _GetChunk (ArchetypeStorage* storage, const TypeList<Args...> *);

			template <typename Fn, typename Chunk, typename ...Types>
			void _WithSingleComponents (Fn &&fn, ArrayView<Chunk> chunks, const Tuple<Types...> *);

			template <typename T>
		ND_ decltype(auto)  _GetSingleComponent ();
	};
	

	
/*
=================================================
	CreateEntity
=================================================
*/
	template <typename ...Components>
	inline EntityID  Registry::CreateEntity ()
	{
		EXLOCK( _drCheck );

		EntityID		id = CreateEntity();
		ArchetypeDesc	desc;
		( desc.Add<Components>(), ... );

		_AddEntity( Archetype{desc}, id );
		return id;
	}
	
/*
=================================================
	AssignComponent
=================================================
*/
	template <typename T>
	inline T&  Registry::AssignComponent (EntityID id)
	{
		EXLOCK( _drCheck );

		ArchetypeStorage*	storage		= null;
		Index_t				index;
		ArchetypeDesc		desc;		desc.Add<T>();
		const ComponentID	comp_id		= ComponentTypeInfo<T>::id;
		
		_entities.GetArchetype( id, OUT storage, OUT index );

		if ( storage )
		{
			if ( auto* comps = storage->GetComponents<T>(); comps )
			{
				// already exists
				return comps[ size_t(index) ];
			}
			else
			{
				// erase
				desc = storage->GetArchetype().Desc();
				
				EntityID	moved;
				storage->Erase( index, OUT moved );

				// update reference to entity that was moved to new index
				if ( moved )
					_entities.SetArchetype( moved, storage, index );

				desc.Add<T>();
			}
		}

		//_messages.Add<Tag_AddedComponent>( id, comp_id );

		storage = null;
		_AddEntity( Archetype{desc}, id, OUT storage, OUT index );
		
		ASSERT( storage );
		ASSERT( storage->GetComponents<T>() );

		return storage->GetComponents<T>()[ size_t(index) ];
	}
		
/*
=================================================
	RemoveComponent
=================================================
*/
	template <typename T>
	inline bool  Registry::RemoveComponent (EntityID id)
	{
		EXLOCK( _drCheck );

		ArchetypeStorage*	src_storage		= null;
		Index_t				src_index;
		ArchetypeDesc		desc;
		const ComponentID	comp_id		= ComponentTypeInfo<T>::id;

		_entities.GetArchetype( id, OUT src_storage, OUT src_index );
		
		if ( not src_storage )
			return false;

		// get new archetype
		{
			desc = src_storage->GetArchetype().Desc();

			bool	found = false;
			if constexpr( IsEmpty<T> )
			{
				for (size_t i = 0; i < desc.tags.size(); ++i)
				{
					if ( desc.tags[i] == comp_id )
					{
						found = true;
						desc.tags.fast_erase( i );
						break;
					}
				}
			}else{
				for (size_t i = 0; i < desc.components.size(); ++i)
				{
					if ( desc.components[i].id == comp_id )
					{
						found = true;
						desc.components.fast_erase( i );
						break;
					}
				}
			}

			if ( not found )
				return false;
		}

		// add entity to new archetype
		ArchetypeStorage*	dst_storage		= null;
		Index_t				dst_index;

		_AddEntity( Archetype{desc}, id, OUT dst_storage, OUT dst_index );
		
		// copy components
		for (auto& comp : desc.components)
		{
			void*	src = src_storage->GetComponents( comp.id );
			void*	dst = dst_storage->GetComponents( comp.id );
			ASSERT( src and dst );

			if ( (src != null) & (dst != null) )
			{
				std::memcpy( OUT dst + BytesU{comp.size} * size_t(dst_index),
							src + BytesU{comp.size} * size_t(src_index),
							 size_t(comp.size) );
			}
		}
		
		//if constexpr( IsEmpty<T> )
		//	_messages.Add<Tag_RemovedComponent>( id, comp_id );
		//else
		//	_messages.Add<Tag_RemovedComponent>( id, std::move( src_storage->GetComponents<T>()[size_t(src_index)] ));
		
		EntityID	moved;
		src_storage->Erase( src_index, OUT moved );
		
		// update reference to entity that was moved to new index
		if ( moved )
			_entities.SetArchetype( moved, src_storage, src_index );

		return true;
	}
	
/*
=================================================
	GetComponent
=================================================
*/
	template <typename T>
	inline T*  Registry::GetComponent (EntityID id)
	{
		EXLOCK( _drCheck );

		ArchetypeStorage*	storage		= null;
		Index_t				index;
		
		_entities.GetArchetype( id, OUT storage, OUT index );

		if ( storage )
		{
			if ( auto* comp = storage->GetComponents<T>() )
			{
				ASSERT( size_t(index) < storage->Count() );
				return comp + size_t(index);
			}
		}

		return null;
	}
//-----------------------------------------------------------------------------
	
/*
=================================================
	AssignSingleComponent
=================================================
*/
	template <typename T>
	inline T&  Registry::AssignSingleComponent ()
	{
		EXLOCK( _drCheck );

		STATIC_ASSERT( not IsEmpty<T> );
		STATIC_ASSERT( std::is_standard_layout_v<T> );
		STATIC_ASSERT( std::is_trivially_copyable_v<T> );
		STATIC_ASSERT( std::is_trivially_destructible_v<T> );
		STATIC_ASSERT( std::is_nothrow_destructible_v<T> );

		auto&	comp = _singleComponents[ TypeIdOf<T>() ];
		if ( not comp.data )
		{
			comp.data		= new T();
			comp.deleter	= [](void* ptr) { delete Cast<T>(ptr); };
		}

		return *Cast<T>( comp.data );
	}
		
/*
=================================================
	RemoveSingleComponent
=================================================
*/
	template <typename T>
	inline bool  Registry::RemoveSingleComponent ()
	{
		EXLOCK( _drCheck );

		return _singleComponents.erase( TypeIdOf<T>() ) > 0;
	}
	
/*
=================================================
	GetSingleComponent
=================================================
*/
	template <typename T>
	inline T*  Registry::GetSingleComponent ()
	{
		EXLOCK( _drCheck );

		auto	iter = _singleComponents.find( TypeIdOf<T>() );
		return	iter != _singleComponents.end() ?
					Cast<T>( iter->second.data ) :
					null;
	}
//-----------------------------------------------------------------------------
	
/*
=================================================
	CheckForDuplicates
=================================================
*/
# ifdef AE_ECS_VALIDATE_SYSTEM_FN
	namespace _reg_detail_
	{
		template <typename RawType, typename WrapedType>
		struct CompareSingleComponents;
		
		template <typename LT, typename RT>
		struct CompareSingleComponents< LT, WriteAccess<RT> >
		{
			static constexpr bool	value = IsSameTypes< LT, RT >;
		};
		
		template <typename LT, typename RT>
		struct CompareSingleComponents< LT, ReadAccess<RT> >
		{
			static constexpr bool	value = IsSameTypes< LT, RT >;
		};
		
		template <typename LT, typename RT>
		struct CompareSingleComponents< LT, OptionalWriteAccess<RT> >
		{
			static constexpr bool	value = IsSameTypes< LT, RT >;
		};
		
		template <typename LT, typename RT>
		struct CompareSingleComponents< LT, OptionalReadAccess<RT> >
		{
			static constexpr bool	value = IsSameTypes< LT, RT >;
		};
		
		template <typename LT, typename ...RTs>
		struct CompareSingleComponents< LT, Subtractive<RTs...> >
		{
			static constexpr bool	value = TypeList<RTs...>::template HasType<LT>;
		};
		
		template <typename LT, typename ...RTs>
		struct CompareSingleComponents< LT, Require<RTs...> >
		{
			static constexpr bool	value = TypeList<RTs...>::template HasType<LT>;
		};
		
		template <typename LT, typename ...RTs>
		struct CompareSingleComponents< LT, RequireAny<RTs...> >
		{
			static constexpr bool	value = TypeList<RTs...>::template HasType<LT>;	// TODO: allow cases with Optional<A> + RequireAny<A,B,C> ???
		};


		template <typename RawTypeList, typename WrapedType>
		struct CompareMultiComponents
		{
			static constexpr bool	value = false;
		};
		
		template <typename LTs, typename ...RTs>
		struct CompareMultiComponents< LTs, Subtractive<RTs...> >
		{
			static constexpr bool	value = TypeList<RTs...>::template ForEach_Or< LTs::template HasType >;
		};
		
		template <typename LTs, typename ...RTs>
		struct CompareMultiComponents< LTs, Require<RTs...> >
		{
			static constexpr bool	value = TypeList<RTs...>::template ForEach_Or< LTs::template HasType >;
		};
		
		template <typename LTs, typename ...RTs>
		struct CompareMultiComponents< LTs, RequireAny<RTs...> >
		{
			static constexpr bool	value = TypeList<RTs...>::template ForEach_Or< LTs::template HasType >;
		};


		template <template <typename, typename> class Comparator,
				  typename RefType, typename ArgsList, size_t ExceptIdx>
		struct CompareComponents
		{
			template <size_t I>
			static constexpr bool  Cmp ()
			{
				if constexpr( I == ExceptIdx )
				{
					return Cmp< I+1 >();
				}
				else
				if constexpr( I < ArgsList::Count )
				{
					return	Comparator< RefType, typename ArgsList::template Get<I> >::value or
							Cmp< I+1 >();
				}
				else
					return false;
			}
		};


		template <typename T>
		struct CheckForDuplicateComponents;

		template <typename T>
		struct CheckForDuplicateComponents< WriteAccess<T> >
		{
			template <size_t I, typename ArgsList>
			static constexpr bool  Test () {
				return not CompareComponents< CompareSingleComponents, T, ArgsList, I >::template Cmp<0>();
			}
		};
		
		template <typename T>
		struct CheckForDuplicateComponents< ReadAccess<T> >
		{
			template <size_t I, typename ArgsList>
			static constexpr bool  Test () {
				return not CompareComponents< CompareSingleComponents, T, ArgsList, I >::template Cmp<0>();
			}
		};
		
		template <typename T>
		struct CheckForDuplicateComponents< OptionalWriteAccess<T> >
		{
			template <size_t I, typename ArgsList>
			static constexpr bool  Test () {
				return not CompareComponents< CompareSingleComponents, T, ArgsList, I >::template Cmp<0>();
			}
		};
		
		template <typename T>
		struct CheckForDuplicateComponents< OptionalReadAccess<T> >
		{
			template <size_t I, typename ArgsList>
			static constexpr bool  Test () {
				return not CompareComponents< CompareSingleComponents, T, ArgsList, I >::template Cmp<0>();
			}
		};

		template <typename ...Types>
		struct CheckForDuplicateComponents< Subtractive<Types...> >
		{
			STATIC_ASSERT( CountOf<Types...>() > 0 );
			
			template <size_t I, typename ArgsList>
			static constexpr bool  Test () {
				return not (CompareComponents< CompareMultiComponents, Types, ArgsList, I >::template Cmp<0>() or ...);
			}
		};

		template <typename ...Types>
		struct CheckForDuplicateComponents< Require<Types...> >
		{
			STATIC_ASSERT( CountOf<Types...>() > 0 );
			
			template <size_t I, typename ArgsList>
			static constexpr bool  Test () {
				return not (CompareComponents< CompareMultiComponents, Types, ArgsList, I >::template Cmp<0>() or ...);
			}
		};

		template <typename ...Types>
		struct CheckForDuplicateComponents< RequireAny<Types...> >
		{
			STATIC_ASSERT( CountOf<Types...>() > 0 );
			
			template <size_t I, typename ArgsList>
			static constexpr bool  Test () {
				return not (CompareComponents< CompareMultiComponents, Types, ArgsList, I >::template Cmp<0>() or ...);
			}
		};

		
		template <typename ArgsList, size_t I = 0>
		static constexpr void  CheckForDuplicates ()
		{
			if constexpr( I < ArgsList::Count )
			{
				constexpr bool is_valid = CheckForDuplicateComponents< typename ArgsList::template Get<I> >::template Test< I, ArgsList >();
				STATIC_ASSERT( is_valid );

				CheckForDuplicates< ArgsList, I+1 >();
			}
		}

		
		
		template <typename LT, typename RT>
		struct SC_Comparator;

		template <typename LT, typename RT>
		struct SC_Comparator< LT, RT* >
		{
			static constexpr bool	value = IsSameTypes< LT, RT >;
		};
		
		template <typename LT, typename RT>
		struct SC_Comparator< LT, RT const* >
		{
			static constexpr bool	value = IsSameTypes< LT, RT >;
		};

		template <typename LT, typename RT>
		struct SC_Comparator< LT, RT& >
		{
			static constexpr bool	value = IsSameTypes< LT, RT >;
		};


		template <typename T>
		struct SC_CheckForDuplicateComponents;
			
		template <typename T>
		struct SC_CheckForDuplicateComponents< T* >
		{
			template <typename ArgsList, size_t I>
			static constexpr bool  Test () {
				return not CompareComponents< SC_Comparator, T, ArgsList, I >::template Cmp<0>();
			}
		};
		
		template <typename T>
		struct SC_CheckForDuplicateComponents< T const* >
		{
			template <typename ArgsList, size_t I>
			static constexpr bool  Test () {
				return not CompareComponents< SC_Comparator, T, ArgsList, I >::template Cmp<0>();
			}
		};
		
		template <typename T>
		struct SC_CheckForDuplicateComponents< T& >
		{
			template <typename ArgsList, size_t I>
			static constexpr bool  Test () {
				return not CompareComponents< SC_Comparator, T, ArgsList, I >::template Cmp<0>();
			}
		};

		template <typename ArgsList, size_t I = 0>
		static constexpr void  SC_CheckForDuplicates ()
		{
			if constexpr( I < ArgsList::Count )
			{
				constexpr bool is_valid = SC_CheckForDuplicateComponents< typename ArgsList::template Get<I> >::template Test< ArgsList, I >();
				STATIC_ASSERT( is_valid );

				SC_CheckForDuplicates< ArgsList, I+1 >();
			}
		}

	}	// _reg_detail_
# endif	// AE_ECS_VALIDATE_SYSTEM_FN
	
/*
=================================================
	SystemFnInfo
=================================================
*/
	namespace _reg_detail_
	{
		template <typename Args, size_t ArgsCount>
		struct _SystemFnInfoImpl;
		
		template <typename Args>
		struct _SystemFnInfoImpl< Args, 1 >
		{
			// check components
			using ChunkArray = typename Args::template Get<0>;
			STATIC_ASSERT( IsSpecializationOf< ChunkArray, ArrayView >);

			using Chunk = typename ChunkArray::value_type;
			using ChunkTL = TypeList< Chunk >;

			STATIC_ASSERT( ChunkTL::Count > 1 );
			STATIC_ASSERT( IsSameTypes< typename ChunkTL::template Get<0>, size_t >);

			using CompOnly = typename ChunkTL::PopFront::type;
			using SCTuple  = Tuple<>;
		};
		
		template <typename Args>
		struct _SystemFnInfoImpl< Args, 2 > : _SystemFnInfoImpl< Args, 1 >
		{
			STATIC_ASSERT( Args::Count == 2 );

			using SCTuple = typename Args::template Get<1>;
			STATIC_ASSERT( IsSpecializationOf< SCTuple, std::tuple >);
		};

		template <typename Fn>
		struct SystemFnInfo
		{
			using _Args		= typename FunctionInfo<Fn>::args;
			using _Info		= _SystemFnInfoImpl< _Args, _Args::Count >;

			using Chunk		= typename _Info::Chunk;
			using CompOnly	= typename _Info::CompOnly;
			using SCTuple	= typename _Info::SCTuple;
		};

	}	// _reg_detail_

/*
=================================================
	Enque
=================================================
*/
	template <typename Class, typename ...Args>
	inline void  Registry::Enque (Class obj, void (Class::*fn)(Args&&...))
	{
		return Enque( [obj, fn](Args&& ...args) { return (obj->*fn)( std::forward<Args>(args)... ); });
	}

	template <typename Fn>
	inline void  Registry::Enque (Fn &&fn)
	{
		EXLOCK( _drCheck );

		_pendingEvents.push_back(
			[this, fn = std::forward<Fn>(fn)] ()
			{
				using Info		= _reg_detail_::SystemFnInfo< Fn >;
				using Chunk		= typename Info::Chunk;
				using CompOnly	= typename Info::CompOnly;
				using SCTuple	= typename Info::SCTuple;
				
				#ifdef AE_ECS_VALIDATE_SYSTEM_FN
					_reg_detail_::CheckForDuplicates< CompOnly >();
					_reg_detail_::SC_CheckForDuplicates< TypeList<SCTuple> >();
				#endif

				//STATIC_ASSERT( Info::valid );

				Array<ArchetypeStorage*>	storages;
				Array<Chunk>				chunks;
				
				for (auto& arch : _archetypes)
				{
					if ( not _IsArchetypeSupported< CompOnly >( arch.first ))
						continue;

					for (auto& st : arch.second)
					{
						st->Lock();
						storages.emplace_back( st.get() );
						chunks.emplace_back( _GetChunk( st.get(), (const CompOnly *)null ));
					}
				}

				_WithSingleComponents( std::move(fn), ArrayView<Chunk>{chunks}, (const SCTuple*)null );
				
				for (auto* st : storages)
				{
					st->Unlock();
				}
			}
		);
	}
//-----------------------------------------------------------------------------
	
/*
=================================================
	ArchetypeCompatibility
=================================================
*/
	namespace _reg_detail_
	{
		template <typename T>
		struct ArchetypeCompatibility;

		template <typename T>
		struct ArchetypeCompatibility< WriteAccess<T> >
		{
			static bool Test (const Archetype &arch) {
				return arch.HasComponent<T>();
			}
		};
		
		template <typename T>
		struct ArchetypeCompatibility< ReadAccess<T> >
		{
			static bool Test (const Archetype &arch) {
				return arch.HasComponent<T>();
			}
		};

		template <typename T>
		struct ArchetypeCompatibility< OptionalWriteAccess<T> >
		{
			static bool Test (const Archetype &arch) {
				return true;
			}
		};
		
		template <typename T>
		struct ArchetypeCompatibility< OptionalReadAccess<T> >
		{
			static bool Test (const Archetype &arch) {
				return true;
			}
		};

		template <typename ...Types>
		struct ArchetypeCompatibility< Subtractive<Types...> >
		{
			STATIC_ASSERT( CountOf<Types...>() > 0 );

			static bool Test (const Archetype &arch) {
				return _Test<Types...>( arch );
			}

			template <typename T, typename ...Next>
			static bool _Test (const Archetype &arch)
			{
				if constexpr( CountOf<Next...>() )
					return not arch.Has<T>() and _Test<Next...>( arch );
				else
					return not arch.Has<T>();
			}
		};

		template <typename ...Types>
		struct ArchetypeCompatibility< Require<Types...> >
		{
			STATIC_ASSERT( CountOf<Types...>() > 0 );

			static bool Test (const Archetype &arch) {
				return _Test<Types...>( arch );
			}

			template <typename T, typename ...Next>
			static bool _Test (const Archetype &arch)
			{
				if constexpr( CountOf<Next...>() )
					return arch.Has<T>() and _Test<Next...>( arch );
				else
					return arch.Has<T>();
			}
		};

		template <typename ...Types>
		struct ArchetypeCompatibility< RequireAny<Types...> >
		{
			STATIC_ASSERT( CountOf<Types...>() > 0 );

			static bool Test (const Archetype &arch) {
				return _Test<Types...>( arch );
			}

			template <typename T, typename ...Next>
			static bool _Test (const Archetype &arch)
			{
				if constexpr( CountOf<Next...>() )
					return arch.Has<T>() or _Test<Next...>( arch );
				else
					return arch.Has<T>();
			}
		};

	}	// _reg_detail_
	
/*
=================================================
	_IsArchetypeSupported
=================================================
*/
	template <typename ArgsList, size_t I>
	inline bool  Registry::_IsArchetypeSupported (const Archetype &arch)
	{
		if constexpr( I < ArgsList::Count )
		{
			using T = typename ArgsList::template Get<I>;

			return	_reg_detail_::ArchetypeCompatibility<T>::Test( arch ) and
					_IsArchetypeSupported<ArgsList, I+1>( arch );
		}
		else
		{
			Unused( arch );
			return true;
		}
	}
	
/*
=================================================
	GetStorageComponent
=================================================
*/
	namespace _reg_detail_
	{
		template <typename T>
		struct GetStorageComponent;

		template <typename T>
		struct GetStorageComponent< WriteAccess<T> >
		{
			static WriteAccess<T>  Get (ArchetypeStorage* storage) {
				return WriteAccess<T>{ storage->GetComponents<T>() };
			}
		};
		
		template <typename T>
		struct GetStorageComponent< ReadAccess<T> >
		{
			static ReadAccess<T>  Get (ArchetypeStorage* storage) {
				return ReadAccess<T>{ storage->GetComponents<T>() };
			}
		};

		template <typename T>
		struct GetStorageComponent< OptionalWriteAccess<T> >
		{
			static OptionalWriteAccess<T>  Get (ArchetypeStorage* storage) {
				return OptionalWriteAccess<T>{ storage->GetComponents<T>() };
			}
		};
		
		template <typename T>
		struct GetStorageComponent< OptionalReadAccess<T> >
		{
			static OptionalReadAccess<T>  Get (ArchetypeStorage* storage) {
				return OptionalReadAccess<T>{ storage->GetComponents<T>() };
			}
		};
		
		template <typename ...Types>
		struct GetStorageComponent< Subtractive<Types...> >
		{
			static Subtractive<Types...>  Get (ArchetypeStorage*) {
				return {};
			}
		};
		
		template <typename ...Types>
		struct GetStorageComponent< Require<Types...> >
		{
			static Require<Types...>  Get (ArchetypeStorage*) {
				return {};
			}
		};
		
		template <typename ...Types>
		struct GetStorageComponent< RequireAny<Types...> >
		{
			static RequireAny<Types...>  Get (ArchetypeStorage*) {
				return {};
			}
		};

	}	// _reg_detail_

/*
=================================================
	_GetChunk
=================================================
*/
	template <typename ...Args>
	inline Tuple<size_t, Args...>  Registry::_GetChunk (ArchetypeStorage* storage, const TypeList<Args...> *)
	{
		return MakeTuple(	storage->Count(),
							_reg_detail_::GetStorageComponent<Args>::Get( storage )... );
	}
	
/*
=================================================
	_GetSingleComponent
=================================================
*/
	template <typename T>
	inline decltype(auto)  Registry::_GetSingleComponent ()
	{
		if constexpr( IsPointer<T> )
		{
			using A = std::remove_pointer_t<T>;
			return GetSingleComponent<A>();		// can be null
		}
		else
		if constexpr( std::is_reference_v<T> )
		{
			using A = std::remove_reference_t<T>;
			ASSERT( GetSingleComponent<A>() );	// TODO: component must be created
			return AssignSingleComponent<A>();
		}
		else
		{
			// error!
			return;
		}
	}
	
/*
=================================================
	_WithSingleComponents
=================================================
*/
	template <typename Fn, typename Chunk, typename ...Types>
	void  Registry::_WithSingleComponents (Fn &&fn, ArrayView<Chunk> chunks, const Tuple<Types...> *)
	{
		if constexpr( CountOf<Types...>() == 0 )
			return fn( chunks );
		else
			return fn( chunks, Tuple<Types...>{ _GetSingleComponent<Types>() ... });
	}
//-----------------------------------------------------------------------------
	

	/*
	template <typename SystemType>
	inline void  Registry::Run (SystemType &&system)
	{
		STATIC_ASSERT( IsBaseOf< System, SystemType >);

		return Run( system._queryId,
					[] (auto ...args) { return system( std::forward<decltype(args)>(args)... ); },
				    std::move(deps) );
	}
	
	template <typename Fn>
	inline void  Registry::Run (const EntityQuery &query, Fn &&fn)
	{

	}
	
	template <typename Ev>
	inline void  Registry::ProcessEvents ()
	{
		CHECK( _pendingEvents.empty() );
		CHECK( _eventQueue.empty() );

		_eventQueue.push_back( [](){ _RunEvent<Ev>(); });

		for (; _eventQueue.size();)
		{
			auto	fn = std::move( _eventQueue.back() );
			_eventQueue.pop_back();

			fn();

			for (auto iter = _pendingEvents.rbegin(); iter != _pendingEvents.rend(); ++iter) {
				_eventQueue.push_back( std::move(*iter) );
			}
			_pendingEvents.clear();
		}
	}

	template <typename Ev>
	inline void  Registry::EnqueEvent ()
	{
		_pendingEvents.push_back( [](){ _RunEvent<BeforeEvent<Ev>>(); });
		_pendingEvents.push_back( [](){ _RunEvent<Ev>(); });
		_pendingEvents.push_back( [](){ _RunEvent<AfterEvent<Ev>>(); });
	}
	
	template <typename Ev>
	inline void  Registry::_RunEvent ()
	{
		TypeId	id	 = TypeIdOf<Ev>();
		auto	iter = _eventListeners.find( id );

		for (; iter != _eventListeners.end() and iter->first == id; ++iter)
		{
			iter->second();
		}
	}
	
	template <typename SystemType, typename Ev>
	inline void  Registry::AddSystem ()
	{
		auto	sys			= new SystemType{};
		bool	inserted	= _systemMap.insert_or_assign( TypeIdOf<SystemType>(), UniquePtr<System>{sys} ).second;
		CHECK( inserted );

		AddEventListener<Ev>( [sys](){ return (*sys)(); });
	}

	template <typename Ev, typename Fn>
	inline void  Registry::AddEventListener (Fn &&fn)
	{
		_eventListeners.insert({ TypeIdOf<Ev>(), EventListener_t{ std::forward<Fn>(fn) }});
	}
	*/

}	// AE::ECS
