// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#pragma once

#include "serializing/Common.h"

namespace AE::Serializing
{

	//
	// Serializable interface
	//
	class ISerializable
	{
	// interface
	public:
		virtual bool Serialize (struct Serializer &) const = 0;
		virtual bool Deserialize (struct Deserializer const&) = 0;
	};


}	// AE::Serializing
