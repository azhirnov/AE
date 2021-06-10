// Copyright (c)  Zhirnov Andrey. For more information see 'LICENSE.txt'

#pragma once

#include "scripting/Impl/ScriptEngine.inl.h"
#include "scriptarray.h"

#include "stl/Math/Vec.h"
#include "stl/Math/Color.h"

namespace AngelScript
{
	class CScriptArray;
}

namespace AE::Scripting
{

	//
	// Core Bindings
	//

	struct CoreBindings final
	{
		static void BindScalarMath (const ScriptEnginePtr &se);
		static void BindVectorMath (const ScriptEnginePtr &se);
		static void BindColor (const ScriptEnginePtr &se);
		static void BindString (const ScriptEnginePtr &se);
		static void BindArray (const ScriptEnginePtr &se);
		static void BindLog (const ScriptEnginePtr &se);
	};


	// Math

	AE_DECL_SCRIPT_TYPE( AE::Math::bool2,		"bool2" );
	AE_DECL_SCRIPT_TYPE( AE::Math::int2,		"int2" );
	AE_DECL_SCRIPT_TYPE( AE::Math::uint2,		"uint2" );
	AE_DECL_SCRIPT_TYPE( AE::Math::float2,		"float2"  );
	AE_DECL_SCRIPT_TYPE( AE::Math::double2,		"double2" );
	
	AE_DECL_SCRIPT_TYPE( AE::Math::bool3,		"bool3" );
	AE_DECL_SCRIPT_TYPE( AE::Math::int3,		"int3" );
	AE_DECL_SCRIPT_TYPE( AE::Math::uint3,		"uint3" );
	AE_DECL_SCRIPT_TYPE( AE::Math::float3,		"float3"  );
	AE_DECL_SCRIPT_TYPE( AE::Math::double3,		"double3" );
	
	AE_DECL_SCRIPT_TYPE( AE::Math::bool4,		"bool4" );
	AE_DECL_SCRIPT_TYPE( AE::Math::int4,		"int4" );
	AE_DECL_SCRIPT_TYPE( AE::Math::uint4,		"uint4" );
	AE_DECL_SCRIPT_TYPE( AE::Math::float4,		"float4"  );
	AE_DECL_SCRIPT_TYPE( AE::Math::double4,		"double4" );
	
	AE_DECL_SCRIPT_TYPE( AE::Math::RGBA32f,		"RGBA32f" );
	AE_DECL_SCRIPT_TYPE( AE::Math::RGBA32u,		"RGBA32u" );
	AE_DECL_SCRIPT_TYPE( AE::Math::RGBA32i,		"RGBA32i" );
	AE_DECL_SCRIPT_TYPE( AE::Math::RGBA8u,		"RGBA8u" );
	AE_DECL_SCRIPT_TYPE( AE::Math::DepthStencil,"DepthStencil" );
	AE_DECL_SCRIPT_TYPE( AE::Math::HSVColor,	"HSVColor" );
	

	// Array
	
	template <typename T>
	class ScriptArray final : protected AngelScript::CScriptArray
	{
	// types
	private:
		static constexpr bool	is_pod	= not ScriptTypeInfo<T>::is_object and not ScriptTypeInfo<T>::is_ref_counted;
	public:
		using iterator			= T *;
		using const_iterator	= T const *;


	// methods
	public:
		ScriptArray () = delete;
		ScriptArray (ScriptArray &&) = delete;
		ScriptArray (const ScriptArray &) = delete;
		
		ScriptArray&  operator = (ScriptArray &&) = delete;
		ScriptArray&  operator = (const ScriptArray &) = delete;

		ND_ usize						size ()					const	{ return this->GetSize(); }
		ND_ bool						empty ()				const	{ return this->IsEmpty(); }

		ND_ EnableIf<is_pod, T *>		data ()							{ return static_cast<T *>( this->GetBuffer() ); };
		ND_ EnableIf<is_pod, T const*>	data ()					const	{ return static_cast<T const*>( const_cast< ScriptArray<T> *>(this)->GetBuffer() ); };

		ND_ EnableIf<is_pod, T *>		begin ()						{ return data(); }
		ND_ EnableIf<is_pod, T const*>	begin ()				const	{ return data(); }
		ND_ EnableIf<is_pod, T *>		end ()							{ return data() + size(); }
		ND_ EnableIf<is_pod, T const*>	end ()					const	{ return data() + size(); }

		ND_ operator ArrayView<T> ()							const	{ return ArrayView<T>{ data(), size() }; }

		ND_ EnableIf<is_pod, T &>		operator [] (usize i)			{ ASSERT( i < size() );  return data()[i]; }
		ND_ EnableIf<is_pod, T const &>	operator [] (usize i)	const	{ ASSERT( i < size() );  return data()[i]; }

		void  push_back (T value)		{ this->InsertLast( &value ); }

		void  clear ()					{ this->Resize( 0 ); }
		void  resize (usize newSize)	{ this->Resize( uint(newSize) ); }
		void  reserve (usize newSize)	{ this->Reserve( uint(newSize) ); }
	};
	

	template <>
	class ScriptArray< String > final : protected AngelScript::CScriptArray
	{
	// types
	public:
		struct iterator
		{
			friend class ScriptArray<String>;

		private:
			ScriptArray<String> *	_arr	= null;
			usize					_index	= UMax;

			iterator (ScriptArray<String> &arr, usize i) : _arr{&arr}, _index{i} {}
			
		public:
			iterator () {}
			iterator (const iterator &) = default;

			ND_ String&			operator * ()					{ ASSERT( _arr ); return (*_arr)[_index]; }
			ND_ String const&	operator * ()			  const	{ ASSERT( _arr ); return (*_arr)[_index]; }

			ND_ bool	operator == (const iterator &rhs) const { return (_arr == rhs._arr) and (_index == rhs._index); }
			ND_ bool	operator != (const iterator &rhs) const { return not (*this == rhs); }

			iterator&			operator ++ ()					{ ++_index;  return *this; }
			iterator			operator ++ (int)				{ iterator res{*this};  ++_index;  return *this; }
		};
		

		struct const_iterator
		{
			friend class ScriptArray<String>;

		private:
			ScriptArray<String> const*	_arr	= null;
			usize						_index	= UMax;

			const_iterator (const ScriptArray<String> &arr, usize i) : _arr{&arr}, _index{i} {}
			
		public:
			const_iterator () {}
			const_iterator (const const_iterator &) = default;
			
			ND_ String const&	operator * ()			{ ASSERT( _arr ); return (*_arr)[_index]; }
			ND_ String const&	operator * ()	const	{ ASSERT( _arr ); return (*_arr)[_index]; }
			
			ND_ bool	operator == (const const_iterator &rhs) const { return (_arr == rhs._arr) and (_index == rhs._index); }
			ND_ bool	operator != (const const_iterator &rhs) const { return not (*this == rhs); }

			const_iterator&		operator ++ ()			{ ++_index;  return *this; }
			const_iterator		operator ++ (int)		{ const_iterator res{*this};  ++_index;  return *this; }
		};


	// methods
	public:
		ScriptArray () = delete;
		ScriptArray (ScriptArray &&) = delete;
		ScriptArray (const ScriptArray &) = delete;
		
		ScriptArray&  operator = (ScriptArray &&) = delete;
		ScriptArray&  operator = (const ScriptArray &) = delete;
		
		ND_ usize			size ()					const	{ return this->GetSize(); }
		ND_ bool			empty ()				const	{ return this->IsEmpty(); }

		ND_ iterator		begin ()						{ return iterator{ *this, 0 }; }
		ND_ const_iterator	begin ()				const	{ return const_iterator{ *this, 0 }; }
		ND_ iterator		end ()							{ return iterator{ *this, size() }; }
		ND_ const_iterator	end ()					const	{ return const_iterator{ *this, size() }; }

		ND_ String &		operator [] (usize i)			{ ASSERT( i < size() );  return *static_cast<String *>( this->At( uint(i) )); }
		ND_ String const &	operator [] (usize i)	const	{ ASSERT( i < size() );  return *static_cast<String const *>( this->At( uint(i) )); }
		
		void  push_back (const String &value)	{ this->InsertLast( const_cast<String *>( &value )); }
		
		void  clear ()							{ this->Resize( 0 ); }
		void  resize (usize newSize)			{ this->Resize( uint(newSize) ); }
		void  reserve (usize newSize)			{ this->Reserve( uint(newSize) ); }
	};


	template <typename T>
	struct ScriptTypeInfo< ScriptArray<T> >
	{
		using type = T;

		static constexpr bool is_object = true;
		static constexpr bool is_ref_counted = false;

		static void Name (INOUT String &s)		{ s+= "array<"; ScriptTypeInfo<T>::Name( INOUT s ); s += ">"; }
		static void ArgName (INOUT String &s)	{ s+= "array<"; ScriptTypeInfo<T>::Name( INOUT s ); s += ">"; }
		static uint SizeOf ()					{ return 0; }
	};


}	// AE::Scripting
