// Copyright (c) 2018-2019,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "networking/Request.h"
#include "threading/Primise/Promise.h"

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
		
		Request  Send (RequestDesc &&desc, StrongDeps &&dependsOn);
		Request  Send (const RequestDesc &desc, StrongDeps &&dependsOn);

		Request  Send (RequestDesc &&desc, WeakDeps &&dependsOn = Default);
		Request  Send (const RequestDesc &desc, WeakDeps &&dependsOn = Default);

		ND_ Promise<Array<uint8_t>>	Download (String url);

	private:
		NetworkManager ();
		~NetworkManager ();
	};


}	// AE::Networking
