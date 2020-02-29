// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "networking/Request.h"
#include "threading/TaskSystem/Promise.h"

namespace AE::Networking
{

	//
	// Network Manager
	//

	class NetworkManager final : public Noncopyable
	{
	// types
	public:
		using MilliSeconds = RequestDesc::MilliSeconds;

		struct Settings
		{
			MilliSeconds	connectionTimeout	{0};
			MilliSeconds	transferTimout		{0};
			MilliSeconds	responseDelay		{60'000};
			MilliSeconds	minFrameTime		{10};
			BytesU			downloadBufferSize	{64u << 10};
		};


	// variables
	private:
		class _InternalImpl;
		UniquePtr< _InternalImpl >		_impl;


	// methods
	public:
		ND_ static NetworkManager&  Instance ();

		bool Setup (const Settings& = Default);
		bool Release ();

		ND_ bool  HasSSL () const;
		
		template <typename ...Deps>
		Request  Send (RequestDesc &&desc, const Tuple<Deps...> &dependsOn = Default);
		
		template <typename ...Deps>
		Request  Send (const RequestDesc &desc, const Tuple<Deps...> &dependsOn = Default);

		ND_ Promise<Array<uint8_t>>	Download (String url);

	private:
		NetworkManager ();
		~NetworkManager ();

		Request  _CreateTask (RequestDesc &&desc);
		Request  _CreateTask (const RequestDesc &desc);
	};
	


/*
=================================================
	Send
=================================================
*/
	template <typename ...Deps>
	inline Request  NetworkManager::Send (RequestDesc &&desc, const Tuple<Deps...> &dependsOn)
	{
		Request	req = _CreateTask( std::move(desc) );
		Scheduler().Run( req, dependsOn );
		return req;
	}
		
	template <typename ...Deps>
	inline Request  NetworkManager::Send (const RequestDesc &desc, const Tuple<Deps...> &dependsOn)
	{
		Request	req = _CreateTask( desc );
		Scheduler().Run( req, dependsOn );
		return req;
	}


}	// AE::Networking
