#ifndef GRADIENTSLIDER_DATA_H
#define GRADIENTSLIDER_DATA_H

#include<exec/types.h>

#define SAVEDS 
#define ASMFUNC __saveds __asm
#define REG(x) register __## x

#define ARGB(r,g,b) ((ULONG)((ULONG)r<<16|(ULONG)g<<8|(ULONG)b))
#define Bit32(x) ((ULONG)(x|(x<<8)|(x<<16)|(x<<24)))
#define Bit8(x) ((UBYTE)((x&0xff000000)>>24))
#define ARGB32(rgb) (ARGB(Bit8(rgb[0]),Bit8(rgb[1]),Bit8(rgb[2])))

struct GRADData
{
	ULONG gd_MaxVal;
	ULONG gd_CurVal;
	ULONG gd_SkipVal;
	UWORD gd_KnobPixels;
	UWORD *gd_PenArray;
	ULONG gd_Freedom;
	Object *gd_FrameImage;
	UBYTE gd_ActiveFromMouse;
	UBYTE gd_DestDeep;
	UBYTE gd_Depth;
};

#endif /* GRADIENTSLIDER_DATA_H */
