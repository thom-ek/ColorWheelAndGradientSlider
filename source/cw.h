#ifndef COLORWHEEL_DATA_H
#define COLORWHEEL_DATA_H

#include<exec/types.h>

#define SAVEDS 
#define ASMFUNC __saveds __asm
#define REG(x) register __## x

ULONG * SAVEDS MakeColorWheel(int,int,ULONG);
ULONG * SAVEDS MakeColorWheel8(struct WHEELData *,int,int,ULONG,UBYTE *);
//ULONG * SAVEDS MakeColorWheel8(struct WHEELData *,struct RastPort *,struct IBox *,int,int,ULONG);
void SAVEDS HSToXY(int, int, int, int, UWORD *, UWORD *);

#define RGB(r,g,b) ((ULONG)((ULONG)r<<16|(ULONG)g<<8|(ULONG)b))
#define Bit32(x) ((ULONG)(x|(x<<8)|(x<<16)|(x<<24)))
#define Bit8(x) ((UBYTE)((x&0xff000000)>>24))

struct WHEELData
{
	struct BitMap *wd_DestBM,*wd_BkgBM;
	struct ColorWheelRGB wd_RGB;
	struct ColorWheelHSB wd_HSB;
	struct Screen *wd_Screen;
	UBYTE wd_Pens[256];
	UBYTE wd_AllocPens[256];
	TEXT wd_Abbrv[7];
	UWORD *wd_Donation;
	UBYTE wd_BevelBox;
	ULONG wd_MaxPens;
	Object *wd_GradientSlider;
	Object *wd_FrameImage;
	UBYTE wd_DestDeep;						// Destination is cybergraphx
	UBYTE wd_Depth;
	UWORD wd_KX,wd_KY;						// current positions of knob
	UWORD wd_ox,wd_oy;						// old positions of knob
	UBYTE wd_ActiveFromMouse;
	UBYTE wd_FirstDraw;
	struct ColorMap *wd_CM;
};

#endif /* COLORWHEEL_DATA_H */
