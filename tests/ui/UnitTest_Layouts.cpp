// Copyright (c) 2018-2020,  Zhirnov Andrey. For more information see 'LICENSE'

#include "ui/LayoutClasses.h"

using namespace AE::UI;

#define TEST	CHECK_FATAL

namespace
{

	struct UpdateLayoutHelper
	{
	// variables
		LayoutPtr				baseLayout;
		ILayout::UpdateQueue	queue;
		LayoutState				viewState;

	// methods
		UpdateLayoutHelper () :
			viewState{ RectF(float2(800.0f, 600.0f)), Default }
		{
		}

		void Process (ILayout* root)
		{
			queue.Clear();

			queue.EnqueueLayout( null, root );
			queue.FlushQueue();

			while ( not queue.HasLayouts() )
			{
				auto curr = queue.ExtractLayout();
			
				auto const&	state = curr.first ? curr.first->State() : viewState;
			
				curr.second->Update( queue, state );
				queue.FlushQueue();
			}
		}

		void RunOnce (LayoutPtr &&child)
		{
			baseLayout.reset( new WidgetLayout( "root" ));

			baseLayout->AddChild( std::move(child) );

			Process( baseLayout.get() );
		}
	};


	static void Test_FixedLayout ()
	{
		UpdateLayoutHelper	helper;

		RectF	r1 = RectF(float2(20.0f, 20.0f)) + float2(300.0f, 400.0f);
		RectF	r2 = RectF(float2(40.0f, 40.0f)) + float2(30.0f, 30.0f);
		RectF	r3 = RectF(float2(40.0f, 40.0f)) + float2(330.0f, 430.0f);

		ILayout*	fl1 = new FixedLayout( "fl1", r1 );
		ILayout*	fl2 = new FixedLayout( "fl2", r2 );

		fl1->AddChild( LayoutPtr(fl2) );

		helper.RunOnce( LayoutPtr(fl1) );

		RectF	r11 = fl1->State().GlobalRect();	TEST(All( r11 == r1 ));
		RectF	r12 = fl1->State().ClippedRect();	TEST(All( r12 == r1 ));

		RectF	r21 = fl2->State().GlobalRect();	TEST(All( r21 == r3 ));
		RectF	r22 = fl2->State().ClippedRect();	TEST(All(Equals( r22.Size(), float2{} )));
	}


	static void Test_StackLayout1 ()
	{
		UpdateLayoutHelper	helper;

		ILayout*	fl = new FixedLayout( "fl", RectF(float2(400.0f, 500.0f)) + float2(100.0f, 20.0f) );
		ILayout*	stl = new StackLayout( "stl", EStackOrigin::Bottom );
		ILayout*	fl1 = new FixedLayout( "fl-1", RectF(float2(100.0f, 200.0f)) );
		ILayout*	fl2 = new FixedLayout( "fl-2", RectF(float2(300.0f, 50.0f)) );
		ILayout*	fl3 = new FixedLayout( "fl-3", RectF(float2(500.0f, 77.0f)) );
		ILayout*	fl4 = new FixedLayout( "fl-4", RectF(float2(400.0f, 150.0f)) );
		ILayout*	fl5 = new FixedLayout( "fl-5", RectF(float2(600.0f, 280.0f)) );

		stl->AddChild( LayoutPtr(fl1) );
		stl->AddChild( LayoutPtr(fl2) );
		stl->AddChild( LayoutPtr(fl3) );
		stl->AddChild( LayoutPtr(fl4) );
		stl->AddChild( LayoutPtr(fl5) );
		fl->AddChild( LayoutPtr{stl} );
	
		helper.RunOnce( LayoutPtr(fl) );

		RectF	r0 = stl->State().GlobalRect();		TEST(All( r0 == RectF(100.0f, 20.0f, 700.0f, 777.0f) ));
		RectF	r1 = fl1->State().GlobalRect();		TEST(All( r1 == RectF(100.0f, 20.0f, 200.0f, 220.0f) ));
		RectF	r2 = fl2->State().GlobalRect();		TEST(All( r2 == RectF(100.0f, 220.0f, 400.0f, 270.0f) ));
		RectF	r3 = fl3->State().GlobalRect();		TEST(All( r3 == RectF(100.0f, 270.0f, 600.0f, 347.0f) ));
		RectF	r4 = fl4->State().GlobalRect();		TEST(All( r4 == RectF(100.0f, 347.0f, 500.0f, 497.0f) ));
		RectF	r5 = fl5->State().GlobalRect();		TEST(All( r5 == RectF(100.0f, 497.0f, 700.0f, 777.0f) ));
	}


	static void Test_StackLayout2 ()
	{
		UpdateLayoutHelper	helper;

		ILayout*	fl  = new FixedLayout( "fl", RectF(float2(400.0f, 500.0f)) + float2(100.0f, 30.0f) );
		ILayout*	stl = new StackLayout( "stl", EStackOrigin::Left );
		ILayout*	fl1 = new FixedLayout( "fl-1", RectF(float2(200.0f, 100.0f)) );
		ILayout*	fl2 = new FixedLayout( "fl-2", RectF(float2( 50.0f, 300.0f)) );
		ILayout*	fl3 = new FixedLayout( "fl-3", RectF(float2( 77.0f, 500.0f)) );
		ILayout*	fl4 = new FixedLayout( "fl-4", RectF(float2(150.0f, 400.0f)) );
		ILayout*	fl5 = new FixedLayout( "fl-5", RectF(float2(280.0f, 600.0f)) );

		stl->AddChild( LayoutPtr(fl1) );
		stl->AddChild( LayoutPtr(fl2) );
		stl->AddChild( LayoutPtr(fl3) );
		stl->AddChild( LayoutPtr(fl4) );
		stl->AddChild( LayoutPtr(fl5) );
		fl->AddChild( LayoutPtr{stl} );
	
		helper.RunOnce( LayoutPtr(fl) );

		RectF	r0 = stl->State().GlobalRect();		TEST(All( r0 == RectF(100.0f, 30.0f, 857.0f, 630.0f) ));
		RectF	r1 = fl1->State().GlobalRect();		TEST(All( r1 == RectF(100.0f, 30.0f, 300.0f, 130.0f) ));
		RectF	r2 = fl2->State().GlobalRect();		TEST(All( r2 == RectF(300.0f, 30.0f, 350.0f, 330.0f) ));
		RectF	r3 = fl3->State().GlobalRect();		TEST(All( r3 == RectF(350.0f, 30.0f, 427.0f, 530.0f) ));
		RectF	r4 = fl4->State().GlobalRect();		TEST(All( r4 == RectF(427.0f, 30.0f, 577.0f, 430.0f) ));
		RectF	r5 = fl5->State().GlobalRect();		TEST(All( r5 == RectF(577.0f, 30.0f, 857.0f, 630.0f) ));
	}


	static void Test_FillStackLayout ()
	{
		UpdateLayoutHelper	helper;
	
		ILayout*			fl  = new FixedLayout( "fl", RectF(float2(400.0f, 560.0f)) + float2(100.0f, 40.0f) );
		FillStackLayout*	fsl = new FillStackLayout( "fsl", EStackOrigin::Bottom );

		ILayout*	al1 = new AlignedLayout( "al-1", ELayoutAlign::Fill, float2() );
		ILayout*	al2 = new AlignedLayout( "al-2", ELayoutAlign::Fill, float2() );
		ILayout*	al3 = new AlignedLayout( "al-3", ELayoutAlign::Fill, float2() );
		ILayout*	al4 = new AlignedLayout( "al-4", ELayoutAlign::Fill, float2() );
	
		fsl->AddChild( LayoutPtr(al1), 1.0f );
		fsl->AddChild( LayoutPtr(al2), 2.0f );
		fsl->AddChild( LayoutPtr(al3), 1.4f );
		fsl->AddChild( LayoutPtr(al4), 1.2f );

		fl->AddChild( LayoutPtr(fsl) );

		helper.RunOnce( LayoutPtr(fl) );

		RectF	r0 = fsl->State().GlobalRect();		TEST(All(Equals( r0, RectF(100.0f,  40.0f, 500.0f, 600.0f), 0.01f )));
		RectF	r1 = al1->State().GlobalRect();		TEST(All(Equals( r1, RectF(100.0f,  40.0f, 500.0f, 140.0f), 0.01f )));
		RectF	r2 = al2->State().GlobalRect();		TEST(All(Equals( r2, RectF(100.0f, 140.0f, 500.0f, 340.0f), 0.01f )));
		RectF	r3 = al3->State().GlobalRect();		TEST(All(Equals( r3, RectF(100.0f, 340.0f, 500.0f, 480.0f), 0.01f )));
		RectF	r4 = al4->State().GlobalRect();		TEST(All(Equals( r4, RectF(100.0f, 480.0f, 500.0f, 600.0f), 0.01f )));
	}


	static void Test_AlignedLayout ()
	{
		UpdateLayoutHelper	helper;

		ILayout*	fl  = new FixedLayout( "fl", RectF(float2(400.0f, 500.0f)) + float2(100.0f, 50.0f) );
		ILayout*	asl = new AutoSizeLayout( "asl" );

		ILayout*	al1 = new AlignedLayout( "al-1", ELayoutAlign::Fill,	float2() );
		ILayout*	al2 = new AlignedLayout( "al-2", ELayoutAlign::FillX,	float2(177.0f) );
		ILayout*	al3 = new AlignedLayout( "al-3", ELayoutAlign::FillY,	float2(233.0f) );
		ILayout*	al4 = new AlignedLayout( "al-4", ELayoutAlign::Center,	float2(44.0f, 78.0f) );
		ILayout*	al5 = new AlignedLayout( "al-5", ELayoutAlign::Bottom,	float2(56.0f, 94.0f) );
		ILayout*	al6 = new AlignedLayout( "al-6", ELayoutAlign::Top,		float2(72.0f, 39.0f) );
		ILayout*	al7 = new AlignedLayout( "al-7", ELayoutAlign::Left,	float2(81.0f, 54.0f) );
		ILayout*	al8 = new AlignedLayout( "al-8", ELayoutAlign::Right,	float2(38.0f, 43.0f) );
	
		ILayout*	al10 = new AlignedLayout( "al-10", ELayoutAlign::FillX   | ELayoutAlign::Bottom, float2(177.0f) );
		ILayout*	al11 = new AlignedLayout( "al-11", ELayoutAlign::FillY   | ELayoutAlign::Left,   float2(233.0f) );
		ILayout*	al12 = new AlignedLayout( "al-12", ELayoutAlign::CenterX | ELayoutAlign::Top,    float2(44.0f, 78.0f) );
		ILayout*	al13 = new AlignedLayout( "al-13", ELayoutAlign::Bottom  | ELayoutAlign::Left,   float2(56.0f, 94.0f) );
		ILayout*	al14 = new AlignedLayout( "al-14", ELayoutAlign::Top     | ELayoutAlign::Right,  float2(72.0f, 39.0f) );
		ILayout*	al15 = new AlignedLayout( "al-15", ELayoutAlign::Left    | ELayoutAlign::Top,    float2(81.0f, 54.0f) );
		ILayout*	al16 = new AlignedLayout( "al-16", ELayoutAlign::Right   | ELayoutAlign::Bottom, float2(38.0f, 43.0f) );
		ILayout*	al17 = new AlignedLayout( "al-17", ELayoutAlign::CenterY | ELayoutAlign::Right,  float2(44.0f, 78.0f) );

		asl->AddChild( LayoutPtr(al1) );
		asl->AddChild( LayoutPtr(al2) );
		asl->AddChild( LayoutPtr(al3) );
		asl->AddChild( LayoutPtr(al4) );
		asl->AddChild( LayoutPtr(al5) );
		asl->AddChild( LayoutPtr(al6) );
		asl->AddChild( LayoutPtr(al7) );
		asl->AddChild( LayoutPtr(al8) );
	
		asl->AddChild( LayoutPtr(al10) );
		asl->AddChild( LayoutPtr(al11) );
		asl->AddChild( LayoutPtr(al12) );
		asl->AddChild( LayoutPtr(al13) );
		asl->AddChild( LayoutPtr(al14) );
		asl->AddChild( LayoutPtr(al15) );
		asl->AddChild( LayoutPtr(al16) );
		asl->AddChild( LayoutPtr(al17) );

		fl->AddChild( LayoutPtr(asl) );

		helper.RunOnce( LayoutPtr(fl) );

		RectF	r1 = al1->State().GlobalRect();		TEST(All( r1 == RectF(100.0f,  50.0f, 500.0f, 550.0f) ));
		RectF	r2 = al2->State().GlobalRect();		TEST(All( r2 == RectF(100.0f,  50.0f, 500.0f,  50.0f) ));
		RectF	r3 = al3->State().GlobalRect();		TEST(All( r3 == RectF(100.0f,  50.0f, 100.0f, 550.0f) ));
		RectF	r4 = al4->State().GlobalRect();		TEST(All( r4 == RectF(278.0f, 261.0f, 322.0f, 339.0f) ));
		RectF	r5 = al5->State().GlobalRect();		TEST(All( r5 == RectF(100.0f,  50.0f, 100.0f, 144.0f) ));
		RectF	r6 = al6->State().GlobalRect();		TEST(All( r6 == RectF(100.0f, 511.0f, 100.0f, 550.0f) ));
		RectF	r7 = al7->State().GlobalRect();		TEST(All( r7 == RectF(100.0f,  50.0f, 181.0f,  50.0f) ));
		RectF	r8 = al8->State().GlobalRect();		TEST(All( r8 == RectF(462.0f,  50.0f, 500.0f,  50.0f) ));

		RectF	r10 = al10->State().GlobalRect();	TEST(All( r10 == RectF(100.0f,  50.0f, 500.0f, 227.0f) ));
		RectF	r11 = al11->State().GlobalRect();	TEST(All( r11 == RectF(100.0f,  50.0f, 333.0f, 550.0f) ));
		RectF	r12 = al12->State().GlobalRect();	TEST(All( r12 == RectF(278.0f, 472.0f, 322.0f, 550.0f) ));
		RectF	r13 = al13->State().GlobalRect();	TEST(All( r13 == RectF(100.0f,  50.0f, 156.0f, 144.0f) ));
		RectF	r14 = al14->State().GlobalRect();	TEST(All( r14 == RectF(428.0f, 511.0f, 500.0f, 550.0f) ));
		RectF	r15 = al15->State().GlobalRect();	TEST(All( r15 == RectF(100.0f, 496.0f, 181.0f, 550.0f) ));
		RectF	r16 = al16->State().GlobalRect();	TEST(All( r16 == RectF(462.0f,  50.0f, 500.0f,  93.0f) ));
		RectF	r17 = al17->State().GlobalRect();	TEST(All( r17 == RectF(456.0f, 261.0f, 500.0f, 339.0f) ));
	}


	static void Test_AutoSizeLayout ()
	{
		UpdateLayoutHelper	helper;

		ILayout*	fl  = new FixedLayout( "fl", RectF(float2(400.0f, 400.0f)) + float2(90.0f, 70.0f) );
		ILayout*	asl = new AutoSizeLayout( "asl" );
		ILayout*	fl1 = new FixedLayout( "fl-1", RectF(float2(50.0f, 50.0f)) + float2(-50.0f, 0.0f) );
		ILayout*	fl2 = new FixedLayout( "fl-2", RectF(float2(100.0f, 100.0f)) + float2(100.0f, 100.0f) );
		ILayout*	fl3 = new FixedLayout( "fl-3", RectF(float2(120.0f, 700.0f)) + float2( 50.0f, 180.0f) );

	
		asl->AddChild( LayoutPtr(fl1) );
		asl->AddChild( LayoutPtr(fl2) );
		asl->AddChild( LayoutPtr(fl3) );
		fl->AddChild( LayoutPtr(asl) );

		helper.RunOnce( LayoutPtr(fl) );

		RectF	r0 = asl->State().GlobalRect();		TEST(All( r0 == RectF(40.0f, 70.0f, 290.0f, 950.0f) ));
	}


	static void Test_FixedMarginLayout ()
	{
		UpdateLayoutHelper	helper;

		ILayout*	fl  = new FixedLayout( "fl", RectF(float2(200.0f)) + float2(100.0f) );
		ILayout*	fml = new FixedMarginLayout( "fml", float2(10.0f), float2(20.0f) );

		fml->AddChild( LayoutPtr(fl) );

		helper.RunOnce( LayoutPtr(fml) );

		RectF	r0 = fl->State().GlobalRect();		TEST(All( r0 == RectF(100.0f, 100.0f, 300.0f, 300.0f) ));
		RectF	r1 = fml->State().GlobalRect();		TEST(All( r1 == RectF( 90.0f,  80.0f, 310.0f, 320.0f) ));
	}


	static void Test_RelativeMarginLayout ()
	{
		UpdateLayoutHelper	helper;

		ILayout*	fl  = new FixedLayout( "fl", RectF(float2(200.0f)) + float2(100.0f) );
		ILayout*	rml = new RelativeMarginLayout( "rml", float2(0.05f), float2(0.1f) );

		rml->AddChild( LayoutPtr(fl) );

		helper.RunOnce( LayoutPtr(rml) );

		RectF	r0 = fl->State().GlobalRect();		TEST(All( r0 == RectF(100.0f, 100.0f, 300.0f, 300.0f) ));
		RectF	r1 = rml->State().GlobalRect();		TEST(All( r1 == RectF( 90.0f,  80.0f, 310.0f, 320.0f) ));
	}


	static void Test_FixedPaddingLayout ()
	{
		UpdateLayoutHelper	helper;

		ILayout*	fl  = new FixedLayout( "fl", RectF(float2(200.0f)) + float2(100.0f) );
		ILayout*	fpl = new FixedPaddingLayout( "fpl", float2(20.0f), float2(10.0f) );

		fl->AddChild( LayoutPtr(fpl) );

		helper.RunOnce( LayoutPtr(fl) );

		RectF	r1 = fpl->State().GlobalRect();		TEST(All( r1 == RectF(120.0f, 110.0f, 280.0f, 290.0f) ));
	}


	static void Test_RelativePaddingLayout ()
	{
		UpdateLayoutHelper	helper;

		ILayout*	fl  = new FixedLayout( "fl", RectF(float2(200.0f)) + float2(100.0f) );
		ILayout*	rpl = new RelativePaddingLayout( "rpl", float2(0.1f), float2(0.05f) );

		fl->AddChild( LayoutPtr(rpl) );

		helper.RunOnce( LayoutPtr(fl) );

		RectF	r1 = rpl->State().GlobalRect();		TEST(All( r1 == RectF(120.0f, 110.0f, 280.0f, 290.0f) ));
	}

}


extern void UnitTest_Layouts ()
{
	Test_FixedLayout();
	Test_StackLayout1();
	Test_StackLayout2();
	Test_FillStackLayout();
	Test_AlignedLayout();
	Test_AutoSizeLayout();
	Test_FixedMarginLayout();
	Test_RelativeMarginLayout();
	Test_FixedPaddingLayout ();
	Test_RelativePaddingLayout();
	
	AE_LOGI( "UnitTest_Layouts - passed" );
}
