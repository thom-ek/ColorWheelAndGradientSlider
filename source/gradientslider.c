/*
** Colorwheel Class
*/

#include<dos.h>
#include<stdlib.h>
#include<m68881.h>
#include<math.h>
#include<string.h>
#include<clib/macros.h>
#include<proto/exec.h>
#include<proto/utility.h>
#include<proto/graphics.h>
#include<proto/intuition.h>
#include<proto/cybergraphics.h>
#include<gadgets/gradientslider.h>
#include<graphics/gfxmacros.h>
#include<exec/memory.h>
#include<libraries/gadtools.h>
#include<intuition/imageclass.h>
#include<intuition/gadgetclass.h>
#include<intuition/intuitionbase.h>
#include"gradientslider.gadget_rev.h"
#include"gs.h"

TEXT version[]=VERSTAG;

#define __XCEXIT 0

/*
** Defines
*/

#define unless(x) if(!(x))
#define GRAD_SetTagArg(tag,id,data)	{ tag.ti_Tag = (ULONG)(id); tag.ti_Data = (ULONG)(data); }
typedef ULONG (*HookFunction)(void);
#define GRADIENTSLIDERGADGET "gradientslider.gadget"

/*
** Prototypes
*/

ULONG ASMFUNC GRAD_Dispatcher(REG(a0) Class *,REG(a2) Object *,REG(a1) Msg);
ULONG GRAD_NEW(Class *,Object *,struct opSet *);
ULONG GRAD_DISPOSE(Class *, Object *, Msg);
ULONG GRAD_SET(Class *, Object *, struct opSet *);
ULONG GRAD_GET(Class *,Object *,struct opGet *);
ULONG GRAD_UPDATE(Class *,Object *,struct opUpdate *);
ULONG GRAD_NOTIFY(Class *,Object *,struct opUpdate *);
ULONG GRAD_RENDER(Class *,Object *,struct gpRender *);
ULONG GRAD_GOACTIVE(Class *,Object *,struct gpInput *);
ULONG GRAD_HANDLEINPUT(Class *,Object *,struct gpInput *);
ULONG GRAD_GOINACTIVE(Class *,Object *,struct gpGoInactive *);
void GRAD_GetGadgetRect( Object *,struct GadgetInfo *,struct Rectangle *);

/*
** Variables
*/

Class *cl=NULL;
struct Library *CyberGfxBase=NULL;

/*
** Create Class
*/

void _dummy(ULONG ble)
{
}

int ASMFUNC __UserLibInit(REG(a6) struct MyLibrary *libbase)
{
	if(IntuitionBase=(struct IntuitionBase *)OpenLibrary("intuition.library",36L))
	{
		if(UtilityBase=OpenLibrary("utility.library",36L))
		{
			if(GfxBase=(struct GfxBase *)OpenLibrary("graphics.library",36L))
			{
				if(CyberGfxBase=OpenLibrary("cybergraphics.library",36L))
				{
					if (cl = MakeClass ("gradientslider.gadget", GADGETCLASS, NULL, sizeof(struct GRADData), 0L))
					{
						cl->cl_Dispatcher.h_Entry = (HookFunction)  GRAD_Dispatcher;
						cl->cl_UserData = (ULONG) libbase;
						AddClass(cl);
						return(FALSE);
					}
					if(CyberGfxBase) CloseLibrary(CyberGfxBase);
				}
				CloseLibrary((struct Library *)GfxBase);
			}
			CloseLibrary(UtilityBase);
		}
		CloseLibrary((struct Library *)IntuitionBase);
	}
	return(TRUE);
}

void ASMFUNC __UserLibCleanup(REG(a6) struct MyLibrary *libbase)
{
	if(cl)
	{
		RemoveClass(cl);
		FreeClass(cl);
	}
	if(IntuitionBase) CloseLibrary((struct Library *)IntuitionBase);
	if(UtilityBase) CloseLibrary(UtilityBase);
	if(GfxBase) CloseLibrary((struct Library *)GfxBase);
	if(CyberGfxBase) CloseLibrary(CyberGfxBase);
}

#define max3(a,b,c) max(max(a,b),c)
#define min3(a,b,c) min(min(a,b),c)

/*
** Dispatcher
*/

ULONG ASMFUNC GRAD_Dispatcher( REG(a0) Class *cl, REG(a2) Object *o, REG(a1) Msg msg)
{
	ULONG retval;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	switch( msg->MethodID )
	{
		case OM_NEW:
			retval = GRAD_NEW(cl, o, (struct opSet *)msg );
			break;
		case OM_DISPOSE:
			retval = GRAD_DISPOSE(cl, o, msg );
			break;
		case OM_SET:
			retval = GRAD_SET(cl, o, (struct opSet *)msg );
			break;
		case OM_GET:
			retval = GRAD_GET(cl, o, (struct opGet *)msg );
			break;
		case OM_UPDATE:
			retval = GRAD_UPDATE(cl, o, (struct opUpdate *)msg );
			break;
		case OM_NOTIFY:
			retval = GRAD_NOTIFY(cl, o, (struct opUpdate *)msg );
			break;
		case GM_RENDER:
			retval = GRAD_RENDER(cl, o, (struct gpRender *)msg );
			break;
		case GM_GOACTIVE:
			retval = GRAD_GOACTIVE(cl, o, (struct gpInput *)msg );
			break;
		case GM_HANDLEINPUT:
			retval = GRAD_HANDLEINPUT(cl, o, (struct gpInput *)msg );
			break;
		case GM_GOINACTIVE:
			retval = GRAD_GOINACTIVE(cl, o, (struct gpGoInactive *)msg );
			break;
		default:
			retval = DoSuperMethodA(cl, o, msg);
			break;
	}
	return(retval);
}

/*
** OM_NEW
*/

ULONG SAVEDS GRAD_NEW(Class *cl,Object *o,struct opSet *ops )
{
	Object *object;
	struct GRADData *GD;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	object = (Object *)DoSuperMethodA( cl, o, (Msg)ops );
	if(object)
	{
		GD = INST_DATA( cl, object );

		GD->gd_MaxVal = GetTagData( GRAD_MaxVal, 0xffff, ops->ops_AttrList );
		GD->gd_CurVal = GetTagData( GRAD_CurVal, 0, ops->ops_AttrList );
		GD->gd_SkipVal = GetTagData( GRAD_SkipVal, 0x1111, ops->ops_AttrList );

		GD->gd_KnobPixels = (UWORD) GetTagData( GRAD_KnobPixels, 5, ops->ops_AttrList );
		GD->gd_PenArray = (UWORD *) GetTagData( GRAD_PenArray, NULL, ops->ops_AttrList );
		GD->gd_Freedom = GetTagData( PGA_FREEDOM, LORIENT_HORIZ, ops->ops_AttrList );

		unless(GD->gd_FrameImage=NewObject(NULL,"frameiclass",
			IA_Recessed,    FALSE,
			IA_EdgesOnly,   TRUE,
			IA_FrameType,   FRAME_BUTTON,
			TAG_END))
		{
			CoerceMethod(cl, o, OM_DISPOSE);
			return NULL;
		}
	}
	return( (ULONG)object );
}

/*
** OM_DISPOSE
*/

ULONG SAVEDS GRAD_DISPOSE(Class *cl, Object *o, Msg msg )
{
	struct GRADData *GD = INST_DATA( cl, o );

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	if(GD->gd_FrameImage) DisposeObject(GD->gd_FrameImage);
	return( DoSuperMethodA(cl, o, msg) );
}

/*
** OM_SET
*/

ULONG SAVEDS GRAD_SET(Class *cl, Object *o, struct opSet *ops)
{
	ULONG retval;
	struct GRADData *GD = INST_DATA( cl, o );
	struct TagItem *tag,*tstate;//, notify;
	struct RastPort *rp;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	retval = DoSuperMethodA( cl, o, (Msg)ops );

	tstate=ops->ops_AttrList;
	while(tag = NextTagItem( &tstate ))
	{
		switch(tag->ti_Tag)
		{
			case GRAD_MaxVal:
				GD->gd_MaxVal=tag->ti_Data;
				retval = FALSE;
				break;
			case GRAD_CurVal:
				GD->gd_CurVal=tag->ti_Data;
				retval = FALSE;
				break;
			case GRAD_SkipVal:
				GD->gd_SkipVal=tag->ti_Data;
				retval = FALSE;
				break;
			case GRAD_PenArray:
				GD->gd_PenArray=(UWORD *)tag->ti_Data;
				retval = FALSE;
				break;
		}
	}

	if(!retval)
	{
		if(rp=ObtainGIRPort( ops->ops_GInfo ))
		{
			DoMethod(o,GM_RENDER,ops->ops_GInfo, rp, GREDRAW_UPDATE);
			ReleaseGIRPort(rp);
		}
	}

	return( retval );
}

/*
** OM_GET
*/

ULONG SAVEDS GRAD_GET(Class *cl,Object *o,struct opGet *opg )
{
	ULONG retval;
	struct GRADData *GD = INST_DATA( cl, o );

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	switch( opg->opg_AttrID )
	{
		case GRAD_MaxVal:
			*opg->opg_Storage = (ULONG)GD->gd_MaxVal;
			retval = TRUE;
			break;
		case GRAD_CurVal:
			*opg->opg_Storage = (ULONG)GD->gd_CurVal;
			retval = TRUE;
			break;
		case GRAD_SkipVal:
			*opg->opg_Storage = (ULONG)GD->gd_SkipVal;
			retval = TRUE;
			break;
		default:
			retval = DoSuperMethodA(cl, o, (Msg)opg);
			break;
	}
	return( retval );
}

/*
** OM_UPDATE
*/

ULONG SAVEDS GRAD_UPDATE(Class *cl,Object *o,struct opUpdate *opu )
{
	ULONG retval,update=FALSE;
	struct GRADData *GD = INST_DATA( cl, o );
	struct TagItem *tag, notify;
	struct RastPort *rp;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	retval = DoSuperMethodA(cl, o, (Msg) opu);

	if( opu->opu_AttrList )
	{
		if(tag = FindTagItem( GRAD_CurVal, opu->opu_AttrList ))
		{
			if( tag->ti_Data != GD->gd_CurVal)
			{
				GD->gd_CurVal = tag->ti_Data;
				update=TRUE;
			}
		}
		if(tag = FindTagItem( GRAD_MaxVal, opu->opu_AttrList ))
		{
			if( tag->ti_Data != GD->gd_MaxVal)
			{
				GD->gd_MaxVal = tag->ti_Data;
				update=TRUE;
			}
		}
		if(tag = FindTagItem( GRAD_SkipVal, opu->opu_AttrList ))
		{
			if( tag->ti_Data != GD->gd_SkipVal)
			{
				GD->gd_SkipVal = tag->ti_Data;
				update=TRUE;
			}
		}
		if(tag = FindTagItem( GRAD_PenArray, opu->opu_AttrList ))
		{
			GD->gd_PenArray = (UWORD *)tag->ti_Data;
			update=TRUE;
		}
		if(update)
		{
			rp = ObtainGIRPort( opu->opu_GInfo );
			if( rp )
			{
				DoMethod( o, GM_RENDER, opu->opu_GInfo, rp, GREDRAW_UPDATE );
				ReleaseGIRPort( rp );
			}
		}
		/* Notify the change. */
		GRAD_SetTagArg( notify, TAG_END, NULL );
		DoMethod( o, OM_NOTIFY, &notify, opu->opu_GInfo, 0 );
	}
	return( retval );
}

ULONG SAVEDS GRAD_NOTIFY(Class *cl,Object *o,struct opUpdate *opu )
{
	struct TagItem tags[3];
	struct GRADData *GD = INST_DATA( cl, o );

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	GRAD_SetTagArg(tags[0], GA_ID, ((struct Gadget *)o)->GadgetID);
	GRAD_SetTagArg(tags[1], GRAD_CurVal, GD->gd_CurVal);

	if( opu->opu_AttrList == NULL )
	{
		GRAD_SetTagArg(tags[2], TAG_END, NULL);
	}
	else GRAD_SetTagArg(tags[2], TAG_MORE, opu->opu_AttrList );

	return( DoSuperMethod(cl, o, OM_NOTIFY, tags, opu->opu_GInfo, opu->opu_Flags) );
}

ULONG SAVEDS GRAD_RENDER(Class *cl,Object *o,struct gpRender *gpr )
{
	ULONG retval,State,pens,rgb[4],a,w,h;
	struct Gadget *gad = (struct Gadget *)o;
	struct Rectangle rect;
	struct DrawInfo *dri;
	struct IBox container;
	struct RastPort *RP = gpr->gpr_RPort;
	struct GRADData *GD = INST_DATA( cl, o );
	UWORD BorderWidth, BorderHeight;
	UWORD patterndata[2] = { 0x2222, 0x8888 };
	ULONG TextPen, FillPen, BackgroundPen;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	retval = DoSuperMethodA(cl, o, (Msg) gpr);

	GRAD_GetGadgetRect( o, gpr->gpr_GInfo, &rect );

	container.Left = rect.MinX; container.Top = rect.MinY;
	container.Width = 1 + rect.MaxX - rect.MinX;
	container.Height = 1 + rect.MaxY - rect.MinY;

	dri = gpr->gpr_GInfo->gi_DrInfo;

	if(gad->Flags & GFLG_DISABLED) State = IDS_DISABLED;
	else State = IDS_NORMAL;

	SetAttrs( GD->gd_FrameImage,
		IA_Left,    container.Left,
		IA_Top,     container.Top,
		IA_Width,   container.Width,
		IA_Height,  container.Height,
		TAG_END);
	DrawImageState( RP, (struct Image *)GD->gd_FrameImage, 0, 0, State, dri);

	if( dri )
	{
		TextPen = dri->dri_Pens[TEXTPEN];
		FillPen = dri->dri_Pens[FILLPEN];
		BackgroundPen = dri->dri_Pens[BACKGROUNDPEN];
	}
	else
	{
  	TextPen = 1;
		FillPen = 3;
		BackgroundPen = 0;
	}

	if(GD->gd_PenArray) for(pens=0;pens<GD->gd_PenArray[pens];pens++);
	else pens=0;

	w=container.Width-4;
	h=container.Height-4;

	switch(gpr->gpr_Redraw)
	{
		case GREDRAW_UPDATE:
		case GREDRAW_REDRAW:
		default:
			switch(pens)
			{
				case 0:
					GetRGB32(gpr->gpr_GInfo->gi_Screen->ViewPort.ColorMap,BackgroundPen,1,rgb);
					FillPixelArray(RP,container.Left+2,container.Top+2,w,h,ARGB32(rgb));
					break;
				case 1:
					GetRGB32(gpr->gpr_GInfo->gi_Screen->ViewPort.ColorMap,GD->gd_PenArray[0],1,rgb);
					FillPixelArray(RP,container.Left+2,container.Top+2,w,h,ARGB32(rgb));
					break;
				default:
					for(a=0;a<pens-1;a++)
					{
						GetRGB32(gpr->gpr_GInfo->gi_Screen->ViewPort.ColorMap,GD->gd_PenArray[a],1,rgb);
						switch(GD->gd_Freedom)
						{
							case LORIENT_VERT: FillPixelArray(RP,container.Left+2,container.Top+2+(h/(pens-1))*a,w,h/(pens-1),ARGB32(rgb)); break;
							case LORIENT_HORIZ: FillPixelArray(RP,container.Left+2+(w/(pens-1))*a,container.Top+2,w/(pens-1),h,ARGB32(rgb)); break;
						}
					}
					break;
			}
			switch(GD->gd_Freedom)
			{
				case LORIENT_VERT:
					SetAPen(RP,BackgroundPen);
					Move(RP,container.Left+2,container.Top+1+h-(GD->gd_CurVal*(h-GD->gd_KnobPixels)/GD->gd_MaxVal)-GD->gd_KnobPixels);
					Draw(RP,container.Left+w+1,container.Top+1+h-(GD->gd_CurVal*(h-GD->gd_KnobPixels)/GD->gd_MaxVal)-GD->gd_KnobPixels);
					Move(RP,container.Left+2,container.Top+1+h-(GD->gd_CurVal*(h-GD->gd_KnobPixels)/GD->gd_MaxVal));
					Draw(RP,container.Left+w+1,container.Top+1+h-(GD->gd_CurVal*(h-GD->gd_KnobPixels)/GD->gd_MaxVal));
					SetAPen(RP,TextPen);
					RectFill(RP,container.Left+2,container.Top+1+h-(GD->gd_CurVal*(h-GD->gd_KnobPixels)/GD->gd_MaxVal)-GD->gd_KnobPixels+1,container.Left+w+1,container.Top+2+h-(GD->gd_CurVal*(h-GD->gd_KnobPixels)/GD->gd_MaxVal)-2); break;
					break;
				case LORIENT_HORIZ:
					SetAPen(RP,BackgroundPen);
					Move(RP,container.Left+1+w-(GD->gd_CurVal*(w-GD->gd_KnobPixels)/GD->gd_MaxVal)-GD->gd_KnobPixels,container.Top+2);
					Draw(RP,container.Left+1+w-(GD->gd_CurVal*(w-GD->gd_KnobPixels)/GD->gd_MaxVal)-GD->gd_KnobPixels,container.Top+h+1);
					Move(RP,container.Left+1+w-(GD->gd_CurVal*(w-GD->gd_KnobPixels)/GD->gd_MaxVal),container.Top+2);
					Draw(RP,container.Left+1+w-(GD->gd_CurVal*(w-GD->gd_KnobPixels)/GD->gd_MaxVal),container.Top+h+1);
					SetAPen(RP,TextPen);
					RectFill(RP,container.Left+1+w-(GD->gd_CurVal*(w-GD->gd_KnobPixels)/GD->gd_MaxVal)-GD->gd_KnobPixels+1,container.Top+2,container.Left+2+w-(GD->gd_CurVal*(w-GD->gd_KnobPixels)/GD->gd_MaxVal)-2,container.Top+h+1); break;
					break;
			}
			break;
	}

skip:
	if(State==IDS_DISABLED)
	{
		BorderHeight = 1;
		BorderWidth = (IntuitionBase->LibNode.lib_Version < 39) ? 1 : 2;

		container.Left += BorderWidth;
		container.Top += BorderHeight;
		container.Width = MAX( 1, container.Width - 2*BorderWidth );
		container.Height = MAX( 1, container.Height - 2*BorderHeight );

		SetDrMd(RP,JAM1);
		SetAfPt(RP, patterndata, 1);

		RectFill(RP,  (LONG)container.Left, (LONG)container.Top,
			container.Left + container.Width,
			container.Top + container.Height);
		SetAfPt(RP, NULL, 0 );
	}

	return( retval );
}

ULONG SAVEDS GRAD_GOACTIVE(Class *cl,Object *o,struct gpInput *gpi )
{
	ULONG retval = GMR_MEACTIVE, Left, Top;
	struct RastPort *rp;
	struct GRADData *GD = INST_DATA( cl, o );
	struct GadgetInfo *gi = gpi->gpi_GInfo;
	struct Gadget *gad = (struct Gadget *)o;
	struct Rectangle rect;
	struct IBox container;
	struct TagItem notify;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	/* Test if we are disabled. */
	if(gad->Flags & GFLG_DISABLED) return( GMR_NOREUSE );

	/* Call first our parent class. */
	DoSuperMethodA(cl, o, (Msg)gpi);

	GRAD_GetGadgetRect( o, gpi->gpi_GInfo, &rect );
	container.Left = rect.MinX; container.Top = rect.MinY;
	container.Width = 1 + rect.MaxX - rect.MinX;
	container.Height = 1 + rect.MaxY - rect.MinY;

	/* Chech whether we were activated from mouse or keyboard. */
	GD->gd_ActiveFromMouse = (gpi->gpi_IEvent != NULL);

	/* Select this gadget. */
	gad->Flags |= GFLG_SELECTED;

	if(rp=ObtainGIRPort(gi))
	{
		Left = gi->gi_Domain.Left;
		Top = gi->gi_Domain.Top;

		GRAD_SetTagArg( notify, TAG_END, NULL );
		DoMethod( o, OM_NOTIFY, &notify, gpi->gpi_GInfo, 0 );

		ReleaseGIRPort( rp );
	}
	else retval = GMR_NOREUSE;

	return(retval);
}

ULONG SAVEDS GRAD_HANDLEINPUT(Class *cl,Object *o,struct gpInput *gpi )
{
	ULONG retval = GMR_MEACTIVE,Left,Top;
	struct InputEvent *ie = gpi->gpi_IEvent;
	struct GRADData *GD = INST_DATA(cl, o);
	struct GadgetInfo *gi = gpi->gpi_GInfo;
	struct Rectangle rect;
	struct IBox container;
	struct RastPort *rp;
	struct TagItem notify;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	if( GD->gd_ActiveFromMouse )
	{
		while( ie && (retval == GMR_MEACTIVE) )
		{
			switch( ie->ie_Class )
			{
				case IECLASS_TIMER: // Scroll listview
					break;
				case IECLASS_RAWMOUSE:
					switch(ie->ie_Code)
					{
						case SELECTUP:
							retval=GMR_NOREUSE|GMR_VERIFY;
						case MENUUP:
							retval=GMR_NOREUSE;
							break;
						case IECODE_NOBUTTON:
							GRAD_GetGadgetRect( o, gpi->gpi_GInfo, &rect );
							container.Left = rect.MinX; container.Top = rect.MinY;
							container.Width = 1 + rect.MaxX - rect.MinX;
							container.Height = 1 + rect.MaxY - rect.MinY;
							if(rp=ObtainGIRPort(gi))
							{
								Left = gi->gi_Domain.Left;
								Top = gi->gi_Domain.Top;

								GRAD_SetTagArg( notify, TAG_END, NULL );
								DoMethod( o, OM_NOTIFY, &notify, gpi->gpi_GInfo, OPUF_INTERIM );

								ReleaseGIRPort( rp );
							}
							break;
					}
					break;
			}
			ie = ie->ie_NextEvent;
		}
	}
	return(retval);
}

ULONG SAVEDS GRAD_GOINACTIVE(Class *cl,Object *o,struct gpGoInactive *gpgi )
{
	ULONG retval;
	struct TagItem notify;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	retval = DoSuperMethodA(cl, o, (Msg)gpgi);
	((struct Gadget *)o)->Flags &= ~GFLG_SELECTED;

	GRAD_SetTagArg( notify, TAG_END, NULL );
	DoMethod( o, OM_NOTIFY, &notify, gpgi->gpgi_GInfo, 0);

	return(retval);
}

void SAVEDS GRAD_GetGadgetRect( Object *o,struct GadgetInfo *gi,struct Rectangle *rect )
{
	struct Gadget *gad = (struct Gadget *)o;
	LONG W, H;

	rect->MinX = gad->LeftEdge;
	rect->MinY = gad->TopEdge;
	W = gad->Width;
	H = gad->Height;

	if( gi )
	{
		if( gad->Flags & GFLG_RELRIGHT ) rect->MinX += gi->gi_Domain.Width - 1;
		if( gad->Flags & GFLG_RELBOTTOM ) rect->MinY += gi->gi_Domain.Height - 1;
		if( gad->Flags & GFLG_RELWIDTH ) W += gi->gi_Domain.Width;
		if( gad->Flags & GFLG_RELHEIGHT ) H += gi->gi_Domain.Height;
	}
	rect->MaxX = rect->MinX + W - (W > 0);
	rect->MaxY = rect->MinY + H - (H > 0);
}

