// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "networking/http/HttpRequest.h"
#include "threading/TaskSystem/Promise.h"

namespace AE::Networking
{

	//
	// Http Client
	//

	class HttpClient final : public Noncopyable
	{
	// types
	public:
		struct Settings
		{
			milliseconds	connectionTimeout	{0};
			milliseconds	transferTimout		{0};
			milliseconds	responseDelay		{60'000};
			milliseconds	minFrameTime		{10};
			BytesU			downloadBufferSize	{64u << 10};
		};


	// variables
	private:
		class _InternalImpl;
		UniquePtr< _InternalImpl >		_impl;


	// methods
	public:
		ND_ static HttpClient&  Instance ();

		bool Setup (const Settings& = Default);
		bool Release ();

		ND_ bool  HasSSL () const;
		
		template <typename ...Deps>
		Request  Send (HttpRequestDesc &&desc, const Tuple<Deps...> &dependsOn = Default);
		
		template <typename ...Deps>
		Request  Send (const HttpRequestDesc &desc, const Tuple<Deps...> &dependsOn = Default);

		ND_ Promise<Array<uint8_t>>	Download (String url);

	private:
		HttpClient ();
		~HttpClient ();

		Request  _CreateTask (HttpRequestDesc &&desc);
		Request  _CreateTask (const HttpRequestDesc &desc);
	};
	


/*
=================================================
	Send
=================================================
*/
	template <typename ...Deps>
	inline Request  HttpClient::Send (HttpRequestDesc &&desc, const Tuple<Deps...> &dependsOn)
	{
		Request	req = _CreateTask( std::move(desc) );
		Scheduler().Run( req, dependsOn );
		return req;
	}
		
	template <typename ...Deps>
	inline Request  HttpClient::Send (const HttpRequestDesc &desc, const Tuple<Deps...> &dependsOn)
	{
		Request	req = _CreateTask( desc );
		Scheduler().Run( req, dependsOn );
		return req;
	}


}	// AE::Networking
