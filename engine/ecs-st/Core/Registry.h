// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "ecs-st/Core/ArchetypeStorage.h"
#include "ecs-st/Core/EntityPool.h"
#include "ecs-st/Core/MessageBuilder.h"
#include "ecs-st/Core/ComponentAccessTypes.h"

namespace AE::ECS
{

	//
	// System Event helpers
	//

	template <typename T>
	struct AfterEvent {};

	template <typename T>
	struct BeforeEvent {};



	//
	// Registry
	//

	class Registry final : public std::enable_shared_from_this< Registry >
	{
	// types
	private:
		using ArchetypeStoragePtr	= UniquePtr<ArchetypeStorage>;
		using ArchetypeMap_t		= HashMap< Archetype, ArchetypeStoragePtr >;
		using Index_t				= ArchetypeStorage::Index_t;
		using ArchetypePair_t		= Pair< const Archetype, ArchetypeStoragePtr >;

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

		using SingleCompMap_t	= HashMap< TypeId, SingleCompWrap >;
		using SCAllocator_t		= UntypedAllocator;

		using EventListener_t	= Function< void (Registry &) >;
		using EventListeners_t	= HashMultiMap< TypeId, EventListener_t >;
		using EventQueue_t		= Array< Function< void () >>;

		struct Query
		{
			ArchetypeQueryDesc			desc;
			Array<ArchetypePair_t *>	archetypes;
		};
		using Queries_t			= Array< Query >;


	// variables
	private:
		// entity + components
		EntityPool			_entities;
		ArchetypeMap_t		_archetypes;		// don't erase elements!
		MessageBuilder		_messages;

		// single components
		SingleCompMap_t		_singleComponents;
		//SCAllocator_t		_scAllocator;		// TODO

		EventListeners_t	_eventListeners;

		EventQueue_t		_eventQueue;
		EventQueue_t		_pendingEvents;

		Queries_t			_queries;

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

		ND_ Ptr<Archetype const>  GetArchetype (EntityID id);

		// component
			template <typename T>
			T&  AssignComponent (EntityID id);
		
			template <typename T>
			bool  RemoveComponent (EntityID id);

			template <typename T>
		ND_ Ptr<T>  GetComponent (EntityID id);

			template <typename ...Types>
		ND_ Tuple<Ptr<Types>...>  GetComponenets (EntityID id);
		
			template <typename T>
			void  RemoveComponents (QueryID query);


		// single component
			template <typename T>
			T&  AssignSingleComponent ();
		
			template <typename T>
			bool  RemoveSingleComponent ();

			template <typename T>
		ND_ Ptr<T>  GetSingleComponent ();

			void DestroyAllSingleComponents ();


		// system
			template <typename ...Args>
			ND_ QueryID  CreateQuery ();
			ND_ QueryID  CreateQuery (const ArchetypeQueryDesc &desc);

			template <typename Fn>
			void  Execute (QueryID query, Fn &&fn);

			template <typename Fn>
			void  Enque (QueryID query, Fn &&fn);
			
			template <typename Obj, typename Class, typename ...Args>
			void  Enque (QueryID query, Obj obj, void (Class::*)(Args&&...));

			template <typename Ev>
			void  EnqueEvent ();

			void  Process ();

			template <typename Ev, typename Fn>
			void  AddEventListener (Fn &&fn);

			template <typename Comp, typename Tag, typename Fn>
			void  AddMessageListener (Fn &&fn);

			template <typename Tag>
			void  AddMessage (EntityID id, ComponentID compId);
		
			template <typename Tag, typename Comp>
			void  AddMessage (EntityID id, const Comp& comp);


	private:
			template <typename Ev>
			void  _RunEvent ();

			bool  _RemoveEntity (EntityID id);
			void  _AddEntity (const Archetype &arch, EntityID id, OUT ArchetypeStorage* &storage, OUT Index_t &index);
			void  _AddEntity (const Archetype &arch, EntityID id);
			void  _MoveEntity (const Archetype &arch, EntityID id, ArchetypeStorage* srcStorage, Index_t srcIndex,
							   OUT ArchetypeStorage* &dstStorage, OUT Index_t &dstIndex);

			void  _OnNewArchetype (ArchetypePair_t *);

			static void  _DecreaseStorageSize (ArchetypeStorage *);

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
		
		ArchetypeStorage*	storage = null;
		Index_t				index;
		
		_AddEntity( Archetype{desc}, id, OUT storage, OUT index );
		ASSERT( storage );

		for (auto& comp : desc.components) {
			_messages.Add<MsgTag_AddedComponent>( id, comp.id );
		}

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

		ArchetypeStorage*	src_storage	= null;
		Index_t				src_index;
		ArchetypeDesc		desc;		desc.Add<T>();
		
		_entities.GetArchetype( id, OUT src_storage, OUT src_index );

		if ( src_storage )
		{
			if ( auto* comps = src_storage->GetComponents<T>(); comps )
			{
				// already exists
				return comps[ size_t(src_index) ];
			}
			else
			{
				desc = src_storage->GetArchetype().Desc();
				desc.Add<T>();
			}
		}
		
		_messages.Add<MsgTag_AddedComponent>( id, ComponentTypeInfo<T>::id );
		
		ArchetypeStorage*	dst_storage	= null;
		Index_t				dst_index;
		_MoveEntity( Archetype{desc}, id, src_storage, src_index, OUT dst_storage, OUT dst_index );
		
		return *dst_storage->GetComponent<T>( dst_index );
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

		_entities.GetArchetype( id, OUT src_storage, OUT src_index );
		
		if ( not src_storage )
			return false;

		// get new archetype
		{
			desc = src_storage->GetArchetype().Desc();

			bool				found	= false;
			const ComponentID	comp_id	= ComponentTypeInfo<T>::id;

			for (size_t i = 0; i < desc.components.size(); ++i)
			{
				if ( desc.components[i].id == comp_id )
				{
					found = true;
					desc.components.fast_erase( i );
					break;
				}
			}

			if ( not found )
				return false;
		}

		if constexpr( IsEmpty<T> )
			_messages.Add<MsgTag_RemovedComponent>( id, ComponentTypeInfo<T>::id );
		else
			_messages.Add<MsgTag_RemovedComponent>( id, src_storage->GetComponent<T>( src_index ));

		// add entity to new archetype
		ArchetypeStorage*	dst_storage		= null;
		Index_t				dst_index;

		_MoveEntity( Archetype{desc}, id, src_storage, src_index, OUT dst_storage, OUT dst_index );
		return true;
	}
	
/*
=================================================
	_DecreaseStorageSize
=================================================
*/
	inline void  Registry::_DecreaseStorageSize (ArchetypeStorage *storage)
	{
		if ( storage->Count()*4 < storage->Capacity() )
		{
			storage->Reserve( Max( ECS_Config::InitialtStorageSize, storage->Count()*2 ));
		}
	}

/*
=================================================
	GetComponent
=================================================
*/
	template <typename T>
	inline Ptr<T>  Registry::GetComponent (EntityID id)
	{
		EXLOCK( _drCheck );

		ArchetypeStorage*	storage		= null;
		Index_t				index;
		
		_entities.GetArchetype( id, OUT storage, OUT index );

		return storage ? storage->GetComponent< std::remove_const_t<T> >( index ) : null;
	}
	
/*
=================================================
	GetComponenets
=================================================
*/
	template <typename ...Types>
	inline Tuple<Ptr<Types>...>  Registry::GetComponenets (EntityID id)
	{
		EXLOCK( _drCheck );

		ArchetypeStorage*	storage		= null;
		Index_t				index;
		
		_entities.GetArchetype( id, OUT storage, OUT index );
		
		if ( storage )
		{
			return Tuple<Ptr<Types>...>{ storage->GetComponent< std::remove_const_t<Types> >( index )... };
		}
		return Default;
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
			comp.data	 = new T();
			comp.deleter = [](void* ptr) { delete Cast<T>(ptr); };

			// TODO: message that single component was created ?
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

		// TODO: message that single component was destroyed ?

		return _singleComponents.erase( TypeIdOf<T>() ) > 0;
	}
	
/*
=================================================
	GetSingleComponent
=================================================
*/
	template <typename T>
	inline Ptr<T>  Registry::GetSingleComponent ()
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
			STATIC_ASSERT( IsSpecializationOf< Chunk, Tuple >);

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
			STATIC_ASSERT( IsSpecializationOf< SCTuple, Tuple >);
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
	template <typename Obj, typename Class, typename ...Args>
	inline void  Registry::Enque (QueryID query, Obj obj, void (Class::*fn)(Args&&...))
	{
		return Enque( query, [obj, fn](Args&& ...args) { return (obj->*fn)( std::forward<Args>(args)... ); });
	}

	template <typename Fn>
	inline void  Registry::Enque (QueryID query, Fn &&fn)
	{
		EXLOCK( _drCheck );

		if constexpr( FunctionInfo<Fn>::args::Count == 0 )
		{
			_pendingEvents.push_back( std::forward<Fn>(fn) );
		}
		else
		{
			_pendingEvents.push_back(
				[this, query, fn = std::forward<Fn>(fn)] ()
				{
					Execute( query, std::move(fn) );
				});
		}
	}
	
/*
=================================================
	Execute
=================================================
*/
	template <typename Fn>
	inline void  Registry::Execute (QueryID query, Fn &&fn)
	{
		EXLOCK( _drCheck );

		using Info		= _reg_detail_::SystemFnInfo< Fn >;
		using Chunk		= typename Info::Chunk;
		using CompOnly	= typename Info::CompOnly;
		using SCTuple	= typename Info::SCTuple;
				
		#ifdef AE_ECS_VALIDATE_SYSTEM_FN
			_reg_detail_::CheckForDuplicates< CompOnly >();
			_reg_detail_::SC_CheckForDuplicates< TypeList<SCTuple> >();
		#endif

		Array<ArchetypeStorage*>	storages;
		Array<Chunk>				chunks;
		const auto&					q_data = _queries[ query.Index() ];
				
		for (auto* ptr : q_data.archetypes)
		{
			auto&	arch	= ptr->first;
			auto&	storage	= ptr->second;

			ASSERT( _IsArchetypeSupported< CompOnly >( arch ));

			storage->Lock();
			storages.emplace_back( storage.get() );
			chunks.emplace_back( _GetChunk( storage.get(), (const CompOnly *)null ));
		}

		_WithSingleComponents( std::move(fn), ArrayView<Chunk>{chunks.data(), chunks.size()}, (const SCTuple*)null );
				
		for (auto* st : storages)
		{
			st->Unlock();
		}
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
		
		template <>
		struct ArchetypeCompatibility< ReadAccess<EntityID> >
		{
			static bool Test (const Archetype &) {
				return true;
			}
		};

		template <typename T>
		struct ArchetypeCompatibility< WriteAccess<T> >
		{
			static bool Test (const Archetype &arch) {
				return arch.Exists<T>();
			}
		};
		
		template <typename T>
		struct ArchetypeCompatibility< ReadAccess<T> >
		{
			static bool Test (const Archetype &arch) {
				return arch.Exists<T>();
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
					return not arch.Exists<T>() and _Test<Next...>( arch );
				else
					return not arch.Exists<T>();
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
					return arch.Exists<T>() and _Test<Next...>( arch );
				else
					return arch.Exists<T>();
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
					return arch.Exists<T>() or _Test<Next...>( arch );
				else
					return arch.Exists<T>();
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
		
		template <>
		struct GetStorageComponent< ReadAccess<EntityID> >
		{
			static ReadAccess<EntityID>  Get (ArchetypeStorage* storage) {
				return ReadAccess<EntityID>{ storage->GetEntities() };
			}
		};

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
=================================================
	AddEventListener
=================================================
*/
	template <typename Ev, typename Fn>
	inline void  Registry::AddEventListener (Fn &&fn)
	{
		EXLOCK( _drCheck );
		STATIC_ASSERT( IsEmpty<Ev> );

		_eventListeners.insert({ TypeIdOf<Ev>(), EventListener_t{ std::forward<Fn>(fn) }});
	}
	
/*
=================================================
	EnqueEvent
=================================================
*/
	template <typename Ev>
	inline void  Registry::EnqueEvent ()
	{
		EXLOCK( _drCheck );
		STATIC_ASSERT( IsEmpty<Ev> );

		_pendingEvents.push_back( [this]()
		{
			_RunEvent<BeforeEvent<Ev>>();
			_RunEvent<Ev>();
			_RunEvent<AfterEvent<Ev>>();
		});
	}
	
/*
=================================================
	_RunEvent
=================================================
*/
	template <typename Ev>
	inline void  Registry::_RunEvent ()
	{
		TypeId	id	 = TypeIdOf<Ev>();
		auto	iter = _eventListeners.find( id );

		for (; iter != _eventListeners.end() and iter->first == id; ++iter)
		{
			iter->second( *this );
		}
	}
//-----------------------------------------------------------------------------
	

/*
=================================================
	AddMessageListener
=================================================
*/
	template <typename Comp, typename Tag, typename Fn>
	inline void  Registry::AddMessageListener (Fn &&fn)
	{
		_messages.AddListener<Comp, Tag>( std::forward<Fn>( fn ));
	}
	
/*
=================================================
	AddMessage
=================================================
*/
	template <typename Tag>
	inline void  Registry::AddMessage (EntityID id, ComponentID compId)
	{
		return _messages.Add<Tag>( id, compId );
	}
		
	template <typename Tag, typename Comp>
	inline void  Registry::AddMessage (EntityID id, const Comp& comp)
	{
		return _messages.Add<Tag>( id, comp );
	}
//-----------------------------------------------------------------------------

	
/*
=================================================
	BuildEntityQueryDesc
=================================================
*/
	namespace _reg_detail_
	{
		template <typename T>
		struct BuildEntityQueryDesc;
		
		template <>
		struct BuildEntityQueryDesc< ReadAccess<EntityID> >
		{
			static void  Apply (ArchetypeQueryDesc &) {}
		};

		template <typename T>
		struct BuildEntityQueryDesc< WriteAccess<T> >
		{
			static void  Apply (ArchetypeQueryDesc &desc) {
				desc.required.Add<T>();
			}
		};
		
		template <typename T>
		struct BuildEntityQueryDesc< ReadAccess<T> >
		{
			static void  Apply (ArchetypeQueryDesc &desc) {
				desc.required.Add<T>();
			}
		};

		template <typename T>
		struct BuildEntityQueryDesc< OptionalWriteAccess<T> >
		{
			static void  Apply (ArchetypeQueryDesc &) {}
		};
		
		template <typename T>
		struct BuildEntityQueryDesc< OptionalReadAccess<T> >
		{
			static void  Apply (ArchetypeQueryDesc &) {}
		};
		
		template <typename ...Types>
		struct BuildEntityQueryDesc< Subtractive<Types...> >
		{
			static void  Apply (ArchetypeQueryDesc &desc) {
				(desc.subtractive.Add<Types>(), ...);
			}
		};
		
		template <typename ...Types>
		struct BuildEntityQueryDesc< Require<Types...> >
		{
			static void  Apply (ArchetypeQueryDesc &desc) {
				(desc.required.Add<Types>(), ...);
			}
		};
		
		template <typename ...Types>
		struct BuildEntityQueryDesc< RequireAny<Types...> >
		{
			static void  Apply (ArchetypeQueryDesc &desc) {
				(desc.requireAny.Add<Types>(), ...);
			}
		};

	}	// _reg_detail_

/*
=================================================
	CreateQuery
=================================================
*/
	template <typename ...Args>
	inline QueryID  Registry::CreateQuery ()
	{
		ArchetypeQueryDesc	desc;
		(_reg_detail_::BuildEntityQueryDesc< Args >::Apply( INOUT desc ), ...);

		return CreateQuery( desc );
	}


}	// AE::ECS
