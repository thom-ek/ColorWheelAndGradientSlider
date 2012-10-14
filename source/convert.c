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
#include"cw.h"


UBYTE dither[4][64]=
{
	{1},
	{1,3,4,2},
	{1,9,3,11,13,5,15,7,4,12,2,10,16,8,14,6},
	{1,33, 9,41,3,35,11,43,49,17,57,25,51,19,59,27,13,45,5,37,15,47,7,39,61,29,53,21,63,31,55,23,
	 4,36,12,44,2,34,10,42,52,20,60,28,50,18,58,26,16,48,8,40,14,46,6,38,64,32,56,24,62,30,54,22}
};


int crack_rgb(UBYTE *rgb,int *red_boost,int *grn_boost,int *blu_boost,double *red_lvl,double *grn_lvl,double *blu_lvl)
{
	int base_color,red,grn,blu;
	double fred,fgrn,fblu;

	fred=rgb[3]/36.6;
	fgrn=rgb[2]/36.6;
	fblu=rgb[1]/85.4;
	red=fred;
	grn=fgrn;
	blu=fblu;
	base_color=(red<<5)+(grn<<2)+(blu);

	*red_lvl=fred-red;
	*grn_lvl=fgrn-grn;
	*blu_lvl=fblu-blu;

	*red_boost=red+1;
	*grn_boost=grn+1;
	*blu_boost=blu+1;

	return base_color;
}

void SAVEDS draw_dithered_pixel(struct WHEELData *WD,int i, int j, int w, int h, UBYTE *rgb, UBYTE *lut, int size,UBYTE *pens)
{
	int n,m,q,r,red_lvl,grn_lvl,blu_lvl;
	int color,red_boost,grn_boost,blu_boost;
	double x,y,frl,fgl,fbl;

	x=(double)i/size;
	y=(double)j/size;
	m=(int)(size*((double)(x-(int)x))+0.5);
	n=(int)(size*((double)(y-(int)y))+0.5);
	q=size/2;
	if(size==8) q=3;
	r=m+n*size;
	color=crack_rgb(rgb,&red_boost,&grn_boost,&blu_boost,&frl,&fgl,&fbl);
	red_lvl=size*size*frl;
	grn_lvl=size*size*fgl;
	blu_lvl=size*size*fbl;
	if(dither[q][r]<=red_lvl)
		color=(color&0x1f)+(red_boost<<5);
	if(dither[q][r]<=grn_lvl)
		color=(color&0xe3)+(grn_boost<<2);
	if(dither[q][r]<=blu_lvl)
		color=(color&0xfc)+(blu_boost);
	*lut=pens[color];
}

ULONG * SAVEDS MakeColorWheel8(struct WHEELData *WD,int w,int h,ULONG color0,UBYTE *pens)
{
	ULONG *data24;
	UBYTE *data8;
	int x,y;

	data8=calloc(h*w,sizeof(UBYTE));
	if(!data8) return NULL;
	data24=MakeColorWheel(w,h,color0);
	if(!data24) return NULL;

	for(x=0;x<w;x++)
		for(y=0;y<h;y++)
		{
			draw_dithered_pixel(WD,x,y,w,h,(UBYTE *)(data24+y*w+x),data8+y*w+x,4,pens);
		}

	free(data24);
	return (ULONG *)data8;
}

/*********************************************************************************************/

/*
int adaptive_lut(ULONG *,UBYTE *, UBYTE *,int,int);
int volume_hist(ULONG *, UBYTE *,int,int);
void analyze_row(UBYTE *,int ,UBYTE *);
void bisect_cube(struct CUBE *,struct CUBE *);
void validate_cube(struct CUBE *);
void copy_cube(struct CUBE ,struct CUBE *);
void gen_cube_color_stats(struct CUBE *,UBYTE *,int );
int add_cube_to_list(struct CUBE ,int );
int delete_cube_from_list(int ,int );
int move_cubes_to_lut(int ,UBYTE *,UBYTE *);
void assign_lut_to_colors(UBYTE *,int );
void replace_pixel(UBYTE *,int ,UBYTE *);

struct CUBE
{
	int x0,y0,z0;
	int x1,y1,z1;
	int vol;
	int r_avg,g_avg,b_avg;
	int fsum,cerr;
	double fom;
};

#define ADPTV_MODE 1
int ADPTV_NUM_COLORS=256;
int ADPTV_QUALITY=50;
struct CUBE *CUBE_LIST;

#define ADPTV_ASSIGN_MODE 0

int adaptive_lut(ULONG *data24,UBYTE *array, UBYTE *color_array,int width,int height)
{
	int num_cubes=0,num_vol,colors,most_cubes=ADPTV_NUM_COLORS-1;
	struct CUBE cube,new_cube;

	if(volume_hist(data24,array,width,height)==1) return -1;
	num_vol=(ADPTV_QUALITY*most_cubes)/100;

	if(most_cubes>0)
	{
		cube.x0=0;
		cube.y0=0;
		cube.z0=0;
		cube.x1=32;
		cube.y1=32;
		cube.z1=32;
		gen_cube_color_stats(&cube,array,1);
		num_cubes=add_cube_to_list(cube,num_cubes);
	}		
	while(num_cubes<num_vol)
	{
		if(CUBE_LIST[0].vol<=1) break;
		copy_cube(CUBE_LIST[0],&cube);
		bisect_cube(&cube,&new_cube);
		gen_cube_color_stats(&cube,array,1);
		gen_cube_color_stats(&new_cube,array,1);
		num_cubes=delete_cube_from_list(0,num_cubes);
		if(cube.fsum!=0)
			num_cubes=add_cube_to_list(cube,num_cubes);
		if(new_cube.fsum!=0)
			num_cubes=add_cube_to_list(new_cube,num_cubes);
	}
	while(num_cubes<most_cubes)
	{
		if(CUBE_LIST[0].vol<=1) break;
		copy_cube(CUBE_LIST[0],&cube);
		bisect_cube(&cube,&new_cube);
		gen_cube_color_stats(&cube,array,2);
		gen_cube_color_stats(&new_cube,array,2);
		num_cubes=delete_cube_from_list(0,num_cubes);
		if(cube.fsum!=0)
			num_cubes=add_cube_to_list(cube,num_cubes);
		if(new_cube.fsum!=0)
			num_cubes=add_cube_to_list(new_cube,num_cubes);
	}
	cube.x0=0;
	cube.y0=0;
	cube.z0=0;
	cube.x1=1;
	cube.y1=1;
	cube.z1=1;
	gen_cube_color_stats(&cube,array,1);
	cube.fom=1000000000.0;
	num_cubes=add_cube_to_list(cube,num_cubes);
	colors=move_cubes_to_lut(num_cubes,array,color_array);
	return colors;
}

int volume_hist(ULONG *data24, UBYTE *vhist,int width,int height)
{
	int i,j;
	ULONG *argb;

	for(i=0;i<32768;i++) vhist[i]=0;

	for(j=0;j<height;j++)
	{
		argb=data24+j*width;
		analyze_row((UBYTE *)argb,width,vhist);
	}
	return 0;
}

void analyze_row(UBYTE *byte_buf,int width,UBYTE *vhist)
{
	int sum,i;

	for(i=0;i<width;i++)
	{
		sum=0;
		sum+=(byte_buf[4*i+3])/8<<10;
		sum+=(byte_buf[4*i+2])/8<<5;
		sum+=(byte_buf[4*i+1])/8<<8;
		if(vhist[sum]!=255) vhist[sum]++;
	}
}

void bisect_cube(struct CUBE *pcube,struct CUBE *pnew_cube)
{
	unsigned int dx,dy,dz;

	validate_cube(pcube);
	dx=pcube->x1-pcube->x0;
	dy=pcube->y1-pcube->y0;
	dz=pcube->z1-pcube->z0;

	if((dx>=dy) && (dx>=dz))
	{
		pnew_cube->x0=(pcube->x1+pcube->x0)/2;
		pnew_cube->y0=pcube->y0;
		pnew_cube->z0=pcube->z0;
		pnew_cube->x1=pcube->x1;
		pnew_cube->y1=pcube->y1;
		pnew_cube->z1=pcube->z1;
		pcube->x1=pnew_cube->x0;
		return;
	}
	if((dy>=dx) && (dy>=dz))
	{
		pnew_cube->x0=pcube->x0;
		pnew_cube->y0=(pcube->y1+pcube->y0)/2;
		pnew_cube->z0=pcube->z0;
		pnew_cube->x1=pcube->x1;
		pnew_cube->y1=pcube->y1;
		pnew_cube->z1=pcube->z1;
		pcube->y1=pnew_cube->y0;
		return;
	}
	if((dy>=dx) && (dy>=dz))
	{
		pnew_cube->x0=pcube->x0;
		pnew_cube->y0=pcube->y0;
		pnew_cube->z0=(pcube->z1+pcube->z0)/2;
		pnew_cube->x1=pcube->x1;
		pnew_cube->y1=pcube->y1;
		pnew_cube->z1=pcube->z1;
		pcube->z1=pnew_cube->z0;
		return;
	}
}

void validate_cube(struct CUBE *pcube)
{
	int temp;

	if(pcube->x0>pcube->x1)
	{
		temp=pcube->x0;
		pcube->x0=pcube->x1;
		pcube->x1=temp;
	}
	if(pcube->y0>pcube->y1)
	{
		temp=pcube->y0;
		pcube->y0=pcube->y1;
		pcube->y1=temp;
	}
	if(pcube->z0>pcube->z1)
	{
		temp=pcube->z0;
		pcube->z0=pcube->z1;
		pcube->z1=temp;
	}
}

void copy_cube(struct CUBE cube,struct CUBE *pnew_cube)
{
	memcpy(pnew_cube,&cube,sizeof(struct CUBE));
}

void gen_cube_color_stats(struct CUBE *pcube,UBYTE *array,int mode)
{
	unsigned int i,j,k,freq;
	int dr,dg,db;
	unsigned int sum=0,index,index0,color_error=0;
	int red_avg=0,grn_avg=0,blu_avg=0;

	pcube->vol=(pcube->x1-pcube->x0)*
						 (pcube->y1-pcube->y0)*
						 (pcube->z1-pcube->z0);

	for(k=pcube->x0;k<pcube->x1;k++)
	{
		index0=k<<10;
		for(j=pcube->y0;j<pcube->y1;j++)
		{
			index=index0+(j<<5)+pcube->z0;
			for(i=pcube->z0;i<pcube->z1;i++)
			{
				freq=array[index];
				red_avg+=k*freq;
				grn_avg+=j*freq;
				blu_avg+=i*freq;
				sum+=freq;
				index++;
			}
		}
	}
	if(sum!=0)
	{
		red_avg/=sum;
		grn_avg/=sum;
		blu_avg/=sum;
	}
	else
	{
		red_avg=0;
		grn_avg=0;
		blu_avg=0;
	}
	for(k=pcube->x0;k<pcube->x1;k++)
	{
		index0=k<<10;
		for(j=pcube->y0;j<pcube->y1;j++)
		{
			index=index0+(j<<5)+pcube->z0;
			for(i=pcube->z0;i<pcube->z1;i++)
			{
				freq=array[index];
				dr=(red_avg-(int)k);
				dg=(grn_avg-(int)j);
				db=(blu_avg-(int)i);
				color_error+=freq*sqrt((double)dr*dr+dg*dg+db*db);
				index++;
			}
		}
	}
	pcube->r_avg=red_avg;
	pcube->g_avg=grn_avg;
	pcube->b_avg=blu_avg;
	pcube->fsum=sum;
	pcube->cerr=color_error;

	switch(mode)
	{
		case 1: pcube->fom=(double)pcube->vol; break;
		case 2: pcube->fom=(double)pcube->cerr; break;
		case 3: pcube->fom=(double)pcube->fsum; break;
		case 4: pcube->fom=(double)(pcube->cerr)*(double)(pcube->fsum); break;
		default: pcube->fom=(double)(pcube->vol-1)*(double)(pcube->fsum); break;
	}
}

int add_cube_to_list(struct CUBE new_cube,int n)
{
	int m;
	m=n+1;

	if(n>0)
		while(CUBE_LIST[n-1].fom<=new_cube.fom)
		{
			memcpy(&CUBE_LIST[n],&CUBE_LIST[n-1],sizeof(struct CUBE));
			n--;
			if(n==0) break;
		}

	memcpy(&CUBE_LIST[n],&new_cube,sizeof(struct CUBE));
	return m;
}

int delete_cube_from_list(int p,int n)
{
	if(p>=n) return n;
	while(p<n-1)
	{
		memcpy(&CUBE_LIST[0],&CUBE_LIST[p+1],sizeof(struct CUBE));
		p++;
	}
	return n-1;
}

int move_cubes_to_lut(int num_cubes,UBYTE *array,UBYTE *color_array)
{
	unsigned int i;

	for(i=0;i<num_cubes;i++)
	{
		color_array[3*i+0]=2*CUBE_LIST[i].r_avg;
		color_array[3*i+1]=2*CUBE_LIST[i].g_avg;
		color_array[3*i+2]=2*CUBE_LIST[i].b_avg;
	}
	for(i=num_cubes;i<256;i++)
	{
		color_array[3*i+0]=0;
		color_array[3*i+1]=0;
		color_array[3*i+2]=0;
	}
	assign_lut_to_colors(array,num_cubes);
	return 256;
}

void assign_lut_to_colors(UBYTE *array,int num_cubes)
{
	int red,grn,blu,dr,dg,db,error,last_error;
	int i,j,k,n;
	int index,index0;

	if(!ADPTV_ASSIGN_MODE)
	{
		for(i=0;i<32768;i++)
		{
			if(array[i]!=0)
			{
				red=(i&0x7c00)>>10;
				grn=(i&0x03e0)>>5;
				blu=(i&0x001f);
				last_error=10000;
				for(n=0;n<num_cubes;n++)
				{
					dr=CUBE_LIST[n].r_avg-red;
					dg=CUBE_LIST[n].g_avg-grn;
					db=CUBE_LIST[n].b_avg-blu;
					error=dr*dr+dg*dg+db*db;
					if(error<=last_error)
					{
						array[i]=n;
						last_error=error;
					}
				}
			}
		}
	}
	else
	{
		for(n=0;n<num_cubes;n++)
		{
			for(k=CUBE_LIST[n].x0;k<CUBE_LIST[n].x1;k++)
			{
				index0=k<<10;
				for(j=CUBE_LIST[n].y0;j<CUBE_LIST[n].y1;j++)
				{
					index=index0+(j<<5)+CUBE_LIST[n].z0;
					for(i=CUBE_LIST[n].z0;i<CUBE_LIST[n].z1;i++)
					{
						array[index]=n;
						index++;
					}
				}
			}
		}
	}
}

void replace_pixel(UBYTE *byte_buf,int width,UBYTE *array)
{
	int i,sum;

	if(ADPTV_MODE)
		for(i=0;i<width;i++)
		{
			sum=0;
			sum+=(byte_buf[4*i+3])/8<<10;
			sum+=(byte_buf[4*i+2])/8<<5;
			sum+=(byte_buf[4*i+1])/8;
			byte_buf[i]=array[sum];
		}
	else
		for(i=0;i<width;i++)
		{
			sum=0;
			sum+=(byte_buf[4*i+3])/32<<5;
			sum+=(byte_buf[4*i+2])/32<<2;
			sum+=(byte_buf[4*i+1])/64;
			byte_buf[i]=(UBYTE)sum;
		}
}

ULONG * SAVEDS MakeColorWheel8(struct WHEELData *WD,struct RastPort *RP,struct IBox *container,int w, int h,ULONG color0)
{
	ULONG *data24;
	UBYTE *rgb;
	UBYTE *vhist;
	UBYTE lut[768];
	int a,x,y;

	geta4();

	if(CUBE_LIST=calloc(256,sizeof(struct CUBE)))
	{
		if(data24=MakeColorWheel(w,h,color0))
		{
			if(ADPTV_MODE)
			{
				if(vhist=calloc(32768,2))
				{
					adaptive_lut(data24,vhist,lut,w,h);
					ADPTV_NUM_COLORS=WD->wd_MaxPens;
					for(a=0;a<WD->wd_MaxPens;a++)
						SetRGB32(&WD->wd_Screen->ViewPort,WD->wd_Pens[a],Bit32(lut[3*a]),Bit32(lut[3*a+1]),Bit32(lut[3*a+2]));
	
					for(y=0;y<h;y++)
					{
						rgb=(UBYTE *)(data24+y*w);
						replace_pixel(rgb,w,vhist);
						for(x=0;x<w;x++)
						{
							SetAPen(RP,rgb[x]);
							WritePixel(RP,container->Left+x,container->Top+y);
						}
					}
					free(vhist);
				}
			}
			free(data24);
		}
		free(CUBE_LIST);
	}
	return 0;
}
*/
