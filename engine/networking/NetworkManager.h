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
	// variables
	private:
		class _InternalImpl;
		UniquePtr< _InternalImpl >		_impl;


	// methods
	public:
		ND_ static NetworkManager&  Instance ();

		bool Setup ();
		bool Release ();

		ND_ bool  HasSSL () const;

		Request  Send (RequestDesc &&desc, Array<AsyncTask> &&dependsOn = {});
		Request  Send (const RequestDesc &desc, Array<AsyncTask> &&dependsOn = {});

		ND_ Promise<String>			RequestStr (RequestDesc &&desc);
		ND_ Promise<Array<uint8_t>>	RequestBin (RequestDesc &&desc);

	private:
		NetworkManager ();
		~NetworkManager ();
	};


}	// AE::Networking
