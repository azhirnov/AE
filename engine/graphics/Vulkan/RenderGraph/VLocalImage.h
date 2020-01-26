// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

namespace AE::Graphics
{

	//
	// Vulkan Local Image
	//

	class VLocalImage
	{
	// variables
	private:
		Ptr<VImage const>		_imageData;	// readonly access is thread safe


	// methods
	public:
		VLocalImage () {}

		bool Create (const VImage *buf);
		
		void AddPendingState (EResourceState state, ExeOrderIndex order) const;
		void ResetState (VBarrierManager &) const;
		void CommitBarrier (VBarrierManager &) const;

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
	bool  VLocalImage::Create (const VImage *buf)
	{
		return true;
	}
	
/*
=================================================
	ResetState
=================================================
*/
	void  VLocalImage::ResetState (VBarrierManager &) const
	{
	}
	
/*
=================================================
	CommitBarrier
=================================================
*/
	void  VLocalImage::CommitBarrier (VBarrierManager &) const
	{
	}


}	// AE::Graphics
