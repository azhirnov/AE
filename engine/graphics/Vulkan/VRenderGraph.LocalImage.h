// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

namespace AE::Graphics
{

	//
	// Local Image
	//

	class VRenderGraph::LocalImage
	{
	// variables
	private:
		Ptr<VImage const>		_imageData;	// readonly access is thread safe


	// methods
	public:
		LocalImage () {}

		bool Create (const VImage *buf);
		
		ND_ bool				IsMutable ()	const	{ return true; } // TODO
		ND_ VkImage				Handle ()		const	{ return _imageData->Handle(); }
		ND_ VImage const*		ToGlobal ()		const	{ return _imageData.get(); }
		ND_ ImageDesc const&	Description ()	const	{ return _imageData->Description(); }
		ND_ uint3 const			Dimension ()	const	{ return _imageData->Dimension(); }
	};


/*
=================================================
	Create
=================================================
*/
	bool  VRenderGraph::LocalImage::Create (const VImage *buf)
	{
		return true;
	}

}	// AE::Graphics
