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
#include<gadgets/colorwheel.h>
#include<gadgets/gradientslider.h>
#include<graphics/gfxmacros.h>
#include<exec/memory.h>
#include<libraries/gadtools.h>
#include<intuition/imageclass.h>
#include<intuition/gadgetclass.h>
#include<intuition/intuitionbase.h>
#include"colorwheel.gadget_rev.h"
#include"cw.h"

TEXT version[]=VERSTAG;

#define __XCEXIT 0

/*
** Defines
*/

#define unless(x) if(!(x))
#define WHEEL_SetTagArg(tag,id,data)	{ tag.ti_Tag = (ULONG)(id); tag.ti_Data = (ULONG)(data); }
typedef ULONG (*HookFunction)(void);
#define COLORWHEELGADGET "colorwheel.gadget"
#define CWRGB(rgb) ((ULONG)(((rgb)->cw_Red&0xff0000)|((rgb)->cw_Green&0xff00)|((rgb)->cw_Blue&0xff)))

/*
** Prototypes
*/

ULONG ASMFUNC WHEEL_Dispatcher(REG(a0) Class *,REG(a2) Object *,REG(a1) Msg);
ULONG WHEEL_NEW(Class *,Object *,struct opSet *);
ULONG WHEEL_DISPOSE(Class *, Object *, Msg);
ULONG WHEEL_SET(Class *, Object *, struct opSet *);
ULONG WHEEL_GET(Class *,Object *,struct opGet *);
ULONG WHEEL_UPDATE(Class *,Object *,struct opUpdate *);
ULONG WHEEL_NOTIFY(Class *,Object *,struct opUpdate *);
ULONG WHEEL_RENDER(Class *,Object *,struct gpRender *);
ULONG WHEEL_GOACTIVE(Class *,Object *,struct gpInput *);
ULONG WHEEL_HANDLEINPUT(Class *,Object *,struct gpInput *);
ULONG WHEEL_GOINACTIVE(Class *,Object *,struct gpGoInactive *);
void WHEEL_GetGadgetRect( Object *,struct GadgetInfo *,struct Rectangle *);

/*
** Variables
*/

Class *cl=NULL;
struct Library *CyberGfxBase=NULL;
struct TextAttr Topaz8={"topaz.font",8,FS_NORMAL,FPF_ROMFONT};
struct TextFont *Topaz8Font;

/*
** Create Class
*/

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
					if(Topaz8Font=OpenFont(&Topaz8))
					{
						if (cl = MakeClass ("colorwheel.gadget", GADGETCLASS, NULL, sizeof(struct WHEELData), 0L))
						{
							cl->cl_Dispatcher.h_Entry = (HookFunction)  WHEEL_Dispatcher;
							cl->cl_UserData = (ULONG) libbase;
							AddClass(cl);
							return(FALSE);
						}
						CloseFont(Topaz8Font);
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

double angle(double x,double y)
{
	return (atan2(x,y)+PI)*255.0/(2*PI);
}

void ASMFUNC ConvertHSBToRGB(REG(a0) struct ColorWheelHSB *hsb,REG(a1) struct ColorWheelRGB *rgb)
{
	double r,g,b;
	double h,s,v;
	double i,t,w,f,u;

	h=360.0*(double)Bit8(hsb->cw_Hue)/255.0;
	s=(double)Bit8(hsb->cw_Saturation)/255.0;
	v=(double)Bit8(hsb->cw_Brightness)/255.0;

	h/=60;
	i=floor(h);
	f=h-i;
	t=v*(1-s); u=v*(1-s*f); w=v*(1-s*(1-f));

	switch((ULONG)i)
	{
		case 0: r=v; g=w; b=t; break;
		case 1: r=u; g=v; b=t; break;
		case 2: r=t; g=v; b=w; break;
		case 3: r=t; g=u; b=v; break;
		case 4: r=w; g=t; b=v; break;
		default: r=v; g=t; b=u; break;
	}
	r*=255.0;
	g*=255.0;
	b*=255.0;
	rgb->cw_Red=Bit32((ULONG)r);
	rgb->cw_Green=Bit32((ULONG)g);
	rgb->cw_Blue=Bit32((ULONG)b);
}

void ASMFUNC ConvertRGBToHSB(REG(a0) struct ColorWheelRGB *rgb,REG(a1) struct ColorWheelHSB *hsb)
{
	double r,g,b;
	double h,s,v;
	double maxcol,mincol;

	r=(double)Bit8(rgb->cw_Red)/255.0;
	g=(double)Bit8(rgb->cw_Green)/255.0;
	b=(double)Bit8(rgb->cw_Blue)/255.0;

	maxcol=max3(r,g,b);
	mincol=min3(r,g,b);
	v=maxcol;
	if(maxcol==mincol)
	{
		s=0;
	}
	else
	{
		s=(maxcol-mincol)/maxcol;
		if(mincol==b)
			h=120*(g-mincol)/(r+g-2.0*mincol);
		else
		{
			if(mincol==r)
				h=120.0*(1.0+(b-mincol)/(b+g-2.0*mincol));
			else
				h=120.0*(2.0+(r-mincol)/(r+b-2.0*mincol));
		}
	}
	h=255.0*h/360.0;
	s*=255.0;
	v*=255.0;
	hsb->cw_Hue=Bit32((ULONG)h);
	hsb->cw_Saturation=Bit32((ULONG)s);
	hsb->cw_Brightness=Bit32((ULONG) v);
}

void SAVEDS DrawKnob(struct RastPort *RP,struct WHEELData *WD,BOOL refresh)
{
	struct RastPort wd_RP;

	InitRastPort(&wd_RP);
	wd_RP.BitMap=WD->wd_BkgBM;

	if(refresh) ClipBlit(&wd_RP,0,0,RP,WD->wd_ox-4,WD->wd_oy-4,8,8,0xc0);
	ClipBlit(RP,WD->wd_KX-4,WD->wd_KY-4,&wd_RP,0,0,8,8,0xc0);
	SetAPen(RP,2);
	Move(RP,WD->wd_KX-1,WD->wd_KY);
	Draw(RP,WD->wd_KX+1,WD->wd_KY);
	Move(RP,WD->wd_KX,WD->wd_KY-1);
	Draw(RP,WD->wd_KX,WD->wd_KY+1);
	SetAPen(RP,1);
	Move(RP,WD->wd_KX-2,WD->wd_KY);
	Draw(RP,WD->wd_KX,WD->wd_KY-2);
	Draw(RP,WD->wd_KX+2,WD->wd_KY);
	Draw(RP,WD->wd_KX,WD->wd_KY+2);
	Draw(RP,WD->wd_KX-2,WD->wd_KY);
	Move(RP,WD->wd_KX-3,WD->wd_KY-1);
	Draw(RP,WD->wd_KX-1,WD->wd_KY-3);
	Draw(RP,WD->wd_KX+1,WD->wd_KY-3);
	Draw(RP,WD->wd_KX+3,WD->wd_KY-1);
	Draw(RP,WD->wd_KX+3,WD->wd_KY+1);
	Draw(RP,WD->wd_KX+1,WD->wd_KY+3);
	Draw(RP,WD->wd_KX-1,WD->wd_KY+3);
	Draw(RP,WD->wd_KX-3,WD->wd_KY+1);
	Draw(RP,WD->wd_KX-3,WD->wd_KY-1);
	Move(RP,WD->wd_KX-2,WD->wd_KY-1);
	Draw(RP,WD->wd_KX-1,WD->wd_KY-2);
	Draw(RP,WD->wd_KX+1,WD->wd_KY-2);
	Draw(RP,WD->wd_KX+2,WD->wd_KY-1);
	Draw(RP,WD->wd_KX+2,WD->wd_KY+1);
	Draw(RP,WD->wd_KX+1,WD->wd_KY+2);
	Draw(RP,WD->wd_KX-1,WD->wd_KY+2);
	Draw(RP,WD->wd_KX-2,WD->wd_KY+1);
//	Draw(RP,WD->wd_KX-2,WD->wd_KY-1);
	WD->wd_ox=WD->wd_KX;
	WD->wd_oy=WD->wd_KY;
}

/*
** Dispatcher
*/

ULONG ASMFUNC WHEEL_Dispatcher( REG(a0) Class *cl, REG(a2) Object *o, REG(a1) Msg msg)
{
	ULONG retval;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	switch( msg->MethodID )
	{
		case OM_NEW:
			retval = WHEEL_NEW(cl, o, (struct opSet *)msg );
			break;
		case OM_DISPOSE:
			retval = WHEEL_DISPOSE(cl, o, msg );
			break;
		case OM_SET:
			retval = WHEEL_SET(cl, o, (struct opSet *)msg );
			break;
		case OM_GET:
			retval = WHEEL_GET(cl, o, (struct opGet *)msg );
			break;
		case OM_UPDATE:
			retval = WHEEL_UPDATE(cl, o, (struct opUpdate *)msg );
			break;
		case OM_NOTIFY:
			retval = WHEEL_NOTIFY(cl, o, (struct opUpdate *)msg );
			break;
		case GM_RENDER:
			retval = WHEEL_RENDER(cl, o, (struct gpRender *)msg );
			break;
		case GM_GOACTIVE:
			retval = WHEEL_GOACTIVE(cl, o, (struct gpInput *)msg );
			break;
		case GM_HANDLEINPUT:
			retval = WHEEL_HANDLEINPUT(cl, o, (struct gpInput *)msg );
			break;
		case GM_GOINACTIVE:
			retval = WHEEL_GOINACTIVE(cl, o, (struct gpGoInactive *)msg );
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

UBYTE SAVEDS AllocatePalette(struct WHEELData *WD)
{
	int a,r,g,b;

//	WD->wd_MaxPens=32;
	for(a=0;a<WD->wd_MaxPens;a++)
		if((WD->wd_AllocPens[a]=ObtainPen(WD->wd_Screen->ViewPort.ColorMap,-1,0,0,0,PEN_EXCLUSIVE|PEN_NO_SETCOLOR))==-1) { a--; break; }
	WD->wd_MaxPens=a;

	if(WD->wd_CM=GetColorMap(WD->wd_MaxPens))
	{
		for(a=0;a<WD->wd_MaxPens;a++)
		{
			b=(((a*255/WD->wd_MaxPens)&0x00e0)>>5)*9*4;
			g=(((a*255/WD->wd_MaxPens)&0x001c)>>2)*9*4;
			r=((a*255/WD->wd_MaxPens)&0x0003)*21*4;
			SetRGB32CM(WD->wd_CM,a,Bit32(r),Bit32(g),Bit32(b));
			SetRGB32(&WD->wd_Screen->ViewPort,WD->wd_AllocPens[a],Bit32(r),Bit32(g),Bit32(b));
		}

		for(a=0;a<256;a++)
		{
			b=((a&0x00e0)>>5)*9*4;
			g=((a&0x001c)>>2)*9*4;
			r=(a&0x0003)*21*4;
			WD->wd_Pens[a]=FindColor(WD->wd_CM,Bit32(r),Bit32(g),Bit32(b),-1);
			WD->wd_Pens[a]=WD->wd_AllocPens[WD->wd_Pens[a]];
		}
	}
	return 1;

/*	if(WD->wd_Donation)
	{
		for(a=0;a<256;a++)
		{
			if(WD->wd_Donation[a]!=-1) WD->wd_Pens[a]=WD->wd_Donation[a];
			else return (UBYTE)a;
		}
	}
	else
	{
		for(a=0;a<WD->wd_MaxPens;a++)
		{
			n=ObtainPen(WD->wd_Screen->ViewPort.ColorMap,-1,0,0,0,PEN_NO_SETCOLOR|PEN_EXCLUSIVE);
			if(n!=-1) WD->wd_Pens[a]=n;
			else break;
		}
		return (UBYTE)a;
	}
*/
}

void SAVEDS FreePalette(struct WHEELData *WD)
{
	int a;

	if(WD->wd_CM) FreeColorMap(WD->wd_CM);
	for(a=0;a<WD->wd_MaxPens;a++) ReleasePen(WD->wd_Screen->ViewPort.ColorMap,WD->wd_AllocPens[a]);

/*	if(!WD->wd_Donation)
		for(a=0;a<WD->wd_MaxPens;a++)
			if(WD->wd_Pens[a]!=-1) ReleasePen(WD->wd_Screen->ViewPort.ColorMap,WD->wd_Pens[a]);
*/
}

ULONG SAVEDS WHEEL_NEW(Class *cl,Object *o,struct opSet *ops )
{
	Object *object;
	struct WHEELData *WD;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	object = (Object *)DoSuperMethodA( cl, o, (Msg)ops );
	if(object)
	{
		WD = INST_DATA( cl, object );

		WD->wd_HSB.cw_Hue = GetTagData( WHEEL_Hue, Bit32(0x0), ops->ops_AttrList );
		WD->wd_HSB.cw_Saturation = GetTagData( WHEEL_Saturation, Bit32(0xff), ops->ops_AttrList );
		WD->wd_HSB.cw_Brightness = GetTagData( WHEEL_Brightness, Bit32(0xff), ops->ops_AttrList );
		CopyMem((APTR)GetTagData( WHEEL_HSB, (ULONG) &WD->wd_HSB, ops->ops_AttrList ),&WD->wd_HSB,sizeof(struct ColorWheelHSB));
		WD->wd_RGB.cw_Red = GetTagData( WHEEL_Red, Bit32(0xff), ops->ops_AttrList );
		WD->wd_RGB.cw_Green = GetTagData( WHEEL_Green, Bit32(0x0), ops->ops_AttrList );
		WD->wd_RGB.cw_Blue = GetTagData( WHEEL_Blue, Bit32(0x0), ops->ops_AttrList );
		CopyMem((APTR)GetTagData( WHEEL_RGB, (ULONG) &WD->wd_RGB, ops->ops_AttrList ),&WD->wd_RGB,sizeof(struct ColorWheelRGB));
		WD->wd_Screen = (struct Screen *)GetTagData( WHEEL_Screen, NULL, ops->ops_AttrList );
		if(!WD->wd_Screen)
		{
			CoerceMethod(cl, o, OM_DISPOSE);
			return NULL;
		}
		strcpy(WD->wd_Abbrv,(char *)GetTagData( WHEEL_Abbrv, (ULONG) "GCBMRY", ops->ops_AttrList));
		WD->wd_Donation = (UWORD *)GetTagData( WHEEL_Donation, NULL, ops->ops_AttrList );
		WD->wd_BevelBox = GetTagData( WHEEL_BevelBox, FALSE, ops->ops_AttrList );
		WD->wd_MaxPens = GetTagData( WHEEL_MaxPens, 255, ops->ops_AttrList );
		WD->wd_GradientSlider = (Object *) GetTagData( WHEEL_GradientSlider, NULL, ops->ops_AttrList );

		WD->wd_Depth=GetBitMapAttr(WD->wd_Screen->RastPort.BitMap,BMA_DEPTH);
		if(WD->wd_Depth>8) WD->wd_DestDeep=TRUE; else WD->wd_DestDeep=FALSE;

		unless(WD->wd_BkgBM=AllocBitMap(8,8,WD->wd_Depth,BMF_CLEAR|BMF_MINPLANES,WD->wd_Screen->RastPort.BitMap))
		{
			CoerceMethod(cl, o, OM_DISPOSE);
			return NULL;
		}

//		WD->wd_DestDeep=!WD->wd_DestDeep;

		if(!WD->wd_DestDeep) AllocatePalette(WD);

		WD->wd_FirstDraw=FALSE;

		if(WD->wd_BevelBox)
		{
			unless(WD->wd_FrameImage=NewObject(NULL,"frameiclass",
				IA_Recessed,    FALSE,
				IA_EdgesOnly,   TRUE,
				IA_FrameType,   FRAME_BUTTON,
				TAG_END))
			{
				CoerceMethod(cl, o, OM_DISPOSE);
				return NULL;
			}
		}
	}
	return( (ULONG)object );
}

/*
** OM_DISPOSE
*/

ULONG SAVEDS WHEEL_DISPOSE(Class *cl, Object *o, Msg msg )
{
	struct WHEELData *WD = INST_DATA( cl, o );
//	int a;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	if(!WD->wd_DestDeep) FreePalette(WD);
//	if(!WD->wd_DestDeep && WD->wd_Screen)
//		for(a=0;a<256;a++)
//			ReleasePen(WD->wd_Screen->ViewPort.ColorMap,WD->wd_Pens[a]);
	if(WD->wd_BkgBM) FreeBitMap(WD->wd_BkgBM);
	if(WD->wd_FrameImage) DisposeObject(WD->wd_FrameImage);
	return( DoSuperMethodA(cl, o, msg) );
}

/*
** OM_SET
*/

ULONG SAVEDS WHEEL_SET(Class *cl, Object *o, struct opSet *ops)
{
	ULONG retval;
	struct WHEELData *WD = INST_DATA( cl, o );
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
			case WHEEL_Hue:
				WD->wd_HSB.cw_Hue=tag->ti_Data;
				ConvertHSBToRGB(&WD->wd_HSB,&WD->wd_RGB);
				retval = FALSE;
				break;
			case WHEEL_Saturation:
				WD->wd_HSB.cw_Saturation=tag->ti_Data;
				ConvertHSBToRGB(&WD->wd_HSB,&WD->wd_RGB);
				retval = FALSE;
				break;
			case WHEEL_Brightness:
				WD->wd_HSB.cw_Brightness=tag->ti_Data;
				ConvertHSBToRGB(&WD->wd_HSB,&WD->wd_RGB);
				retval = FALSE;
				break;
			case WHEEL_HSB:
				if(tag->ti_Data) CopyMem((APTR)tag->ti_Data,&WD->wd_HSB,sizeof(struct ColorWheelHSB));
				ConvertHSBToRGB(&WD->wd_HSB,&WD->wd_RGB);
				retval = FALSE;
				break;
			case WHEEL_Red:
				WD->wd_RGB.cw_Red=tag->ti_Data;
				ConvertRGBToHSB(&WD->wd_RGB,&WD->wd_HSB);
				retval = FALSE;
				break;
			case WHEEL_Green:
				WD->wd_RGB.cw_Green=tag->ti_Data;
				ConvertRGBToHSB(&WD->wd_RGB,&WD->wd_HSB);
				retval = FALSE;
				break;
			case WHEEL_Blue:
				WD->wd_RGB.cw_Blue=tag->ti_Data;
				ConvertRGBToHSB(&WD->wd_RGB,&WD->wd_HSB);
				retval = FALSE;
				break;
			case WHEEL_RGB:
				if(tag->ti_Data) CopyMem((APTR)tag->ti_Data,&WD->wd_RGB,sizeof(struct ColorWheelRGB));
				ConvertRGBToHSB(&WD->wd_RGB,&WD->wd_HSB);
				retval = FALSE;
				break;
			case WHEEL_GradientSlider:
				WD->wd_GradientSlider = (Object *)tag->ti_Data;
				retval = TRUE;
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

ULONG SAVEDS WHEEL_GET(Class *cl,Object *o,struct opGet *opg )
{
	ULONG retval;
	struct WHEELData *WD = INST_DATA( cl, o );

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	switch( opg->opg_AttrID )
	{
		case WHEEL_Hue:
			*opg->opg_Storage = (ULONG)WD->wd_HSB.cw_Hue;
			retval = TRUE;
			break;
		case WHEEL_Saturation:
			*opg->opg_Storage = (ULONG)WD->wd_HSB.cw_Saturation;
			retval = TRUE;
			break;
		case WHEEL_Brightness:
			*opg->opg_Storage = (ULONG)WD->wd_HSB.cw_Brightness;
			retval = TRUE;
			break;
		case WHEEL_HSB:
			if(opg->opg_Storage) CopyMem(&WD->wd_HSB,opg->opg_Storage,sizeof(struct ColorWheelHSB));
			retval = TRUE;
			break;
		case WHEEL_Red:
			*opg->opg_Storage = (ULONG)WD->wd_RGB.cw_Red;
			retval = TRUE;
			break;
		case WHEEL_Green:
			*opg->opg_Storage = (ULONG)WD->wd_RGB.cw_Green;
			retval = TRUE;
			break;
		case WHEEL_Blue:
			*opg->opg_Storage = (ULONG)WD->wd_RGB.cw_Blue;
			retval = TRUE;
			break;
		case WHEEL_RGB:
			if(opg->opg_Storage) CopyMem(&WD->wd_RGB,opg->opg_Storage,sizeof(struct ColorWheelRGB));
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

ULONG SAVEDS WHEEL_UPDATE(Class *cl,Object *o,struct opUpdate *opu )
{
	ULONG retval,update=FALSE;
	struct WHEELData *WD = INST_DATA( cl, o );
	struct TagItem *tag, notify;
	struct RastPort *rp;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	retval = DoSuperMethodA(cl, o, (Msg) opu);

	if( opu->opu_AttrList )
	{
		if(tag = FindTagItem( WHEEL_Hue, opu->opu_AttrList ))
		{
			if( tag->ti_Data != WD->wd_HSB.cw_Hue)
			{
				WD->wd_HSB.cw_Hue = tag->ti_Data;
				ConvertHSBToRGB(&WD->wd_HSB,&WD->wd_RGB);
				update=TRUE;
			}
		}
		if(tag = FindTagItem( WHEEL_Saturation, opu->opu_AttrList ))
		{
			if( tag->ti_Data != WD->wd_HSB.cw_Saturation)
			{
				WD->wd_HSB.cw_Saturation = tag->ti_Data;
				ConvertHSBToRGB(&WD->wd_HSB,&WD->wd_RGB);
				update=TRUE;
			}
		}
		if(tag = FindTagItem( WHEEL_Brightness, opu->opu_AttrList ))
		{
			if( tag->ti_Data != WD->wd_HSB.cw_Brightness)
			{
				WD->wd_HSB.cw_Brightness = tag->ti_Data;
				ConvertHSBToRGB(&WD->wd_HSB,&WD->wd_RGB);
				update=TRUE;
			}
		}
		if(tag = FindTagItem( WHEEL_HSB, opu->opu_AttrList ))
		{
			if(tag->ti_Data) CopyMem((APTR)tag->ti_Data,&WD->wd_HSB,sizeof(struct ColorWheelHSB));
			ConvertHSBToRGB(&WD->wd_HSB,&WD->wd_RGB);
			update=TRUE;
		}
		if(tag = FindTagItem( WHEEL_Red, opu->opu_AttrList ))
		{
			if( tag->ti_Data != WD->wd_RGB.cw_Red)
			{
				WD->wd_RGB.cw_Red = tag->ti_Data;
				ConvertRGBToHSB(&WD->wd_RGB,&WD->wd_HSB);
				update=TRUE;
			}
		}
		if(tag = FindTagItem( WHEEL_Green, opu->opu_AttrList ))
		{
			if( tag->ti_Data != WD->wd_RGB.cw_Green)
			{
				WD->wd_RGB.cw_Green= tag->ti_Data;
				ConvertRGBToHSB(&WD->wd_RGB,&WD->wd_HSB);
				update=TRUE;
			}
		}
		if(tag = FindTagItem( WHEEL_Blue, opu->opu_AttrList ))
		{
			if( tag->ti_Data != WD->wd_RGB.cw_Blue)
			{
				WD->wd_RGB.cw_Blue= tag->ti_Data;
				ConvertRGBToHSB(&WD->wd_RGB,&WD->wd_HSB);
				update=TRUE;
			}
		}
		if(tag = FindTagItem( WHEEL_RGB, opu->opu_AttrList ))
		{
			if(tag->ti_Data) CopyMem((APTR)tag->ti_Data,&WD->wd_RGB,sizeof(struct ColorWheelRGB));
			ConvertRGBToHSB(&WD->wd_RGB,&WD->wd_HSB);
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
		WHEEL_SetTagArg( notify, TAG_END, NULL );
		DoMethod( o, OM_NOTIFY, &notify, opu->opu_GInfo, 0 );
	}
	return( retval );
}

ULONG SAVEDS WHEEL_NOTIFY(Class *cl,Object *o,struct opUpdate *opu )
{
	struct TagItem tags[8];
	struct WHEELData *WD = INST_DATA( cl, o );

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	WHEEL_SetTagArg(tags[0], GA_ID, ((struct Gadget *)o)->GadgetID);
	WHEEL_SetTagArg(tags[1], WHEEL_Hue, WD->wd_HSB.cw_Hue);
	WHEEL_SetTagArg(tags[2], WHEEL_Saturation, WD->wd_HSB.cw_Saturation);
//	WHEEL_SetTagArg(tags[3], WHEEL_Brightness, WD->wd_HSB.cw_Brightness);
	WHEEL_SetTagArg(tags[3], WHEEL_Red, WD->wd_RGB.cw_Red);
	WHEEL_SetTagArg(tags[4], WHEEL_Green, WD->wd_RGB.cw_Green);
	WHEEL_SetTagArg(tags[5], WHEEL_Blue, WD->wd_RGB.cw_Blue);

	if( opu->opu_AttrList == NULL )
	{
		WHEEL_SetTagArg(tags[6], TAG_END, NULL);
	}
	else WHEEL_SetTagArg(tags[6], TAG_MORE, opu->opu_AttrList );

	return( DoSuperMethod(cl, o, OM_NOTIFY, tags, opu->opu_GInfo, opu->opu_Flags) );
}

ULONG SAVEDS WHEEL_RENDER(Class *cl,Object *o,struct gpRender *gpr )
{
	ULONG retval,State,w,h,*data,rgb[3];
	struct Gadget *gad = (struct Gadget *)o;
	struct Rectangle rect;
	struct DrawInfo *dri;
	struct IBox container;
	struct RastPort *RP = gpr->gpr_RPort;
	struct WHEELData *WD = INST_DATA( cl, o );
	UWORD BorderWidth, BorderHeight;
	UWORD patterndata[2] = { 0x2222, 0x8888 };
	ULONG TextPen, FillPen, BackgroundPen;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	retval = DoSuperMethodA(cl, o, (Msg) gpr);

	WHEEL_GetGadgetRect( o, gpr->gpr_GInfo, &rect );

	container.Left = rect.MinX; container.Top = rect.MinY;
	container.Width = 1 + rect.MaxX - rect.MinX;
	container.Height = 1 + rect.MaxY - rect.MinY;

	dri = gpr->gpr_GInfo->gi_DrInfo;

	if(gad->Flags & GFLG_DISABLED) State = IDS_DISABLED;
	else State = IDS_NORMAL;

	if(WD->wd_BevelBox)
	{
		SetAttrs( WD->wd_FrameImage,
			IA_Left,    container.Left,
			IA_Top,     container.Top,
			IA_Width,   container.Width,
			IA_Height,  container.Height,
			TAG_END);
		DrawImageState( RP, (struct Image *)WD->wd_FrameImage, 0, 0, State, dri);
		w=container.Width-16;
		h=container.Height-16;
	}
	else
	{
		w=container.Width;
		h=container.Height;
	}

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

	if(WD->wd_BevelBox)
	{
		container.Left+=8;
		container.Top+=8;
	}
	
	switch(gpr->gpr_Redraw)
	{
		case GREDRAW_UPDATE:
			if(WD->wd_GradientSlider)
			{
				ULONG maximum;
				GetAttr(GRAD_MaxVal,WD->wd_GradientSlider,&maximum);
				SetGadgetAttrs((struct Gadget *)WD->wd_GradientSlider,gpr->gpr_GInfo->gi_Window,NULL,GRAD_CurVal,(Bit8(WD->wd_HSB.cw_Brightness)*maximum)/255,TAG_DONE);
			}
			break;
		case GREDRAW_REDRAW:
		default:
			if(WD->wd_GradientSlider)
			{
				ULONG maximum;
				GetAttr(GRAD_MaxVal,WD->wd_GradientSlider,&maximum);
				SetGadgetAttrs((struct Gadget *)WD->wd_GradientSlider,gpr->gpr_GInfo->gi_Window,NULL,GRAD_CurVal,(Bit8(WD->wd_HSB.cw_Brightness)*maximum)/255,TAG_DONE);
			}
			if(w>1 && h>1)
			{
				if(WD->wd_DestDeep)
				{
					GetRGB32(WD->wd_Screen->ViewPort.ColorMap,BackgroundPen,1,rgb);
					data=MakeColorWheel(w,h,0xff000000|RGB(rgb[0],rgb[1],rgb[2]));
					WritePixelArray(data,0,0,w*4,RP,container.Left,container.Top,w,h,RECTFMT_ARGB);
					free(data);
				}
				else
				{
					UWORD x,y;
					UBYTE *datab;

					GetRGB32(WD->wd_Screen->ViewPort.ColorMap,BackgroundPen,1,rgb);
					datab=(UBYTE *)MakeColorWheel8(WD,w,h,RGB(rgb[0],rgb[1],rgb[2]),WD->wd_Pens);
					for(x=0;x<w;x++)
						for(y=0;y<h;y++)
						{
							SetAPen(RP,*(datab+y*w+x));
							WritePixel(RP,container.Left+x,container.Top+y);
						}

//					WritePixelArray(data,0,0,w,RP,container.Left,container.Top,w,h,RECTFMT_LUT8);
					free(datab);

/*
					UWORD x,y;
					int a;

					SetAPen(RP,TextPen);
					DrawEllipse(RP,container.Left+w/2,container.Top+h/2,w/2,h/2);
					DrawEllipse(RP,container.Left+w/2,container.Top+h/2,w/2-1,h/2-1);
					for(a=0;a<6;a++)
					{
						SetFont(RP,Topaz8Font);
						HSToXY((a*60*255/360)-(255/12),255,w,h,&x,&y);
						Move(RP,container.Left+w/2,container.Top+h/2);
						Draw(RP,container.Left+x,container.Top+y);
						HSToXY(((a+2)*60*255/360),170,w,h,&x,&y);
						Move(RP,container.Left+x-4,container.Top+y+4);
						Text(RP,&WD->wd_Abbrv[a],1);
					}
*/
				}
			}
			break;
	}

	if(WD->wd_BevelBox)
	{
		container.Left-=8;
		container.Top-=8;
	}

	HSToXY(Bit8(WD->wd_HSB.cw_Hue),Bit8(WD->wd_HSB.cw_Saturation),w,h,&WD->wd_KX,&WD->wd_KY);
	WD->wd_KX+=container.Left;
	WD->wd_KY+=container.Top;
skip:
	DrawKnob(RP,WD,WD->wd_FirstDraw);
	WD->wd_FirstDraw=TRUE;

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

ULONG SAVEDS WHEEL_GOACTIVE(Class *cl,Object *o,struct gpInput *gpi )
{
	ULONG retval = GMR_MEACTIVE, Left, Top;
	LONG s,w,h;
	struct RastPort *rp;
	struct WHEELData *WD = INST_DATA( cl, o );
	struct GadgetInfo *gi = gpi->gpi_GInfo;
	struct Gadget *gad = (struct Gadget *)o;
	struct Rectangle rect;
	struct IBox container;
	struct TagItem notify;
	double xw,yw;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	/* Test if we are disabled. */
	if(gad->Flags & GFLG_DISABLED) return( GMR_NOREUSE );

	/* Call first our parent class. */
	DoSuperMethodA(cl, o, (Msg)gpi);

	WHEEL_GetGadgetRect( o, gpi->gpi_GInfo, &rect );
	container.Left = rect.MinX; container.Top = rect.MinY;
	container.Width = 1 + rect.MaxX - rect.MinX;
	container.Height = 1 + rect.MaxY - rect.MinY;

	/* Chech whether we were activated from mouse or keyboard. */
	WD->wd_ActiveFromMouse = (gpi->gpi_IEvent != NULL);

	/* Select this gadget. */
	gad->Flags |= GFLG_SELECTED;

	if(rp=ObtainGIRPort(gi))
	{
		Left = gi->gi_Domain.Left;
		Top = gi->gi_Domain.Top;

		if(WD->wd_BevelBox)
		{
			w=container.Width-16;
			h=container.Height-16;
		}
		else
		{
			w=container.Width;
			h=container.Height;
		}

		xw=(double)(gpi->gpi_Mouse.X-Left-(WD->wd_BevelBox?8:0))/(double)w-0.5;
		yw=(double)(gpi->gpi_Mouse.Y-Top-(WD->wd_BevelBox?8:0))/(double)h-0.5;

		xw=-xw;

		s=(ULONG)(sqrt(xw*xw+yw*yw)*512.0);
		if(!WD->wd_BevelBox && s>255)
		{
			ReleaseGIRPort( rp );
			retval = GMR_NOREUSE;
			return retval;
		}

		if(s>255) s=255;
		HSToXY((int)angle(xw,yw),s,w,h,&WD->wd_KX,&WD->wd_KY);
		WD->wd_KX+=container.Left;
		WD->wd_KY+=container.Top;

		WD->wd_HSB.cw_Saturation=Bit32(s);
		WD->wd_HSB.cw_Hue=(ULONG)(angle(xw,yw));
		WD->wd_HSB.cw_Hue=Bit32(WD->wd_HSB.cw_Hue);
		ConvertHSBToRGB(&WD->wd_HSB,&WD->wd_RGB);

		DrawKnob(rp,WD,TRUE);
		WHEEL_SetTagArg( notify, TAG_END, NULL );
		DoMethod( o, OM_NOTIFY, &notify, gpi->gpi_GInfo, 0 );

		ReleaseGIRPort( rp );
	}
	else retval = GMR_NOREUSE;

	return(retval);
}

ULONG SAVEDS WHEEL_HANDLEINPUT(Class *cl,Object *o,struct gpInput *gpi )
{
	ULONG retval = GMR_MEACTIVE,Left,Top;
	LONG s,w,h;
	struct InputEvent *ie = gpi->gpi_IEvent;
	struct WHEELData *WD = INST_DATA(cl, o);
	struct GadgetInfo *gi = gpi->gpi_GInfo;
	struct Rectangle rect;
	struct IBox container;
	struct RastPort *rp;
	struct TagItem notify;
	double xw,yw;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	if( WD->wd_ActiveFromMouse )
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
							WHEEL_GetGadgetRect( o, gpi->gpi_GInfo, &rect );
							container.Left = rect.MinX; container.Top = rect.MinY;
							container.Width = 1 + rect.MaxX - rect.MinX;
							container.Height = 1 + rect.MaxY - rect.MinY;
							if(rp=ObtainGIRPort(gi))
							{
								Left = gi->gi_Domain.Left;
								Top = gi->gi_Domain.Top;

								if(WD->wd_BevelBox)
								{
									w=container.Width-16;
									h=container.Height-16;
								}
								else
								{
									w=container.Width;
									h=container.Height;
								}

								xw=(double)(gpi->gpi_Mouse.X-(WD->wd_BevelBox?8:0))/(double)w-0.5;
								yw=(double)(gpi->gpi_Mouse.Y-(WD->wd_BevelBox?8:0))/(double)h-0.5;

								xw=-xw;

								s=(ULONG)(sqrt(xw*xw+yw*yw)*512.0);
								if(s>255) s=255;
								HSToXY((int)angle(xw,yw),s,w,h,&WD->wd_KX,&WD->wd_KY);
								WD->wd_KX+=container.Left;
								WD->wd_KY+=container.Top;

								WD->wd_HSB.cw_Saturation=Bit32(s);
								WD->wd_HSB.cw_Hue=(ULONG)(angle(xw,yw));
								WD->wd_HSB.cw_Hue=Bit32(WD->wd_HSB.cw_Hue);
								ConvertHSBToRGB(&WD->wd_HSB,&WD->wd_RGB);

								DrawKnob(rp,WD,TRUE);
								WHEEL_SetTagArg( notify, TAG_END, NULL );
								DoMethod( o, OM_NOTIFY, &notify, gpi->gpi_GInfo, OPUF_INTERIM );
								ReleaseGIRPort( rp );
							}
							if(WD->wd_GradientSlider)
							{
								ULONG maximum;
								GetAttr(GRAD_MaxVal,WD->wd_GradientSlider,&maximum);
//								SetGadgetAttrs((struct Gadget *)WD->wd_GradientSlider,gpi->gpi_GInfo->gi_Window,NULL,GRAD_CurVal,(Bit8(WD->wd_HSB.cw_Brightness)*maximum)/255,TAG_DONE);
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

ULONG SAVEDS WHEEL_GOINACTIVE(Class *cl,Object *o,struct gpGoInactive *gpgi )
{
	ULONG retval;
	struct TagItem notify;
	struct RastPort *rp;

	putreg( REG_A6, cl->cl_UserData );
	geta4();

	retval = DoSuperMethodA(cl, o, (Msg)gpgi);
	((struct Gadget *)o)->Flags &= ~GFLG_SELECTED;

	WHEEL_SetTagArg( notify, TAG_END, NULL );
	DoMethod( o, OM_NOTIFY, &notify, gpgi->gpgi_GInfo, 0);

	rp = ObtainGIRPort( gpgi->gpgi_GInfo );
	if( rp )
	{
		DoMethod( o, GM_RENDER, gpgi->gpgi_GInfo, rp, GREDRAW_UPDATE );
		ReleaseGIRPort( rp );
	}

	return(retval);
}

void SAVEDS WHEEL_GetGadgetRect( Object *o,struct GadgetInfo *gi,struct Rectangle *rect )
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

