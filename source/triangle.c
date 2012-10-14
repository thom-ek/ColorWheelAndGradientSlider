/*
	RGB Triangle
	(C)1998 Artur Muszynski AMST/UNION
*/

#include<m68881.h>
#include<math.h>
#include<stdlib.h>
#include<exec/types.h>

/* linearized channels */
#define LINCHAN 4

#define MAKERGB(r,g,b)	(((r)<<16) | ((g)<<8) | (b))

typedef struct Vertex
{
	int x,y;		// screen position
	int r,g,b;	// RGB color
	int pa,pb,pc;
} Vertex;

typedef struct Lin
{
	int v,dv,h,l,iv;
} Lin;



static void __inline InitLin(Lin *l, int v1, int v2, int dx)
{
	l->v  = v1;
	l->dv = abs(v2 - v1) % dx;
	l->h  = (v2-v1)/dx;
	l->l  = dx>>1;
	l->iv = (v2-v1)>0 ? 1 : -1;
}


static void __inline StepLinArray(Lin *l, int minch, int maxch, int dx)
{
	int ch;

	l += minch;
	for(ch=minch; ch++<=maxch; l++)
	{
		l->l += l->dv;
		l->v += l->h;
		if(l->l > dx)
		{
			l->l -= dx;
			l->v += l->iv;
		}
	}
}



static void DrawLinear(ULONG *bline, Lin *l1, Lin *l2)
{
	int x,x1,x2,dx;
	Lin l[LINCHAN];
	int i;

	dx = (x2=l2[0].v) - (x1=l1[0].v);
	if(dx>0)
	{
		for(i=1; i<=3; i++)	InitLin(&l[i],l1[i].v, l2[i].v, dx);
		for(x=x1; x<x2; x++)
		{
			bline[x] = MAKERGB(l[1].v, l[2].v, l[3].v);
			StepLinArray(l, 1, 3, dx);
		}
	}
}



static void DrawTriangle(ULONG *bmap, int w, int h, Vertex *a, Vertex *b, Vertex *c)
{
	Vertex *t;
	int y;
	int dy1,dy2;
	Lin l1[LINCHAN],l2[LINCHAN];
	ULONG *bline;

	/* y-sort verticles */
	if(a->y>b->y) {t=a; a=b; b=t;}
	if(a->y>c->y) {t=a; a=c; c=t;}
	if(b->y>c->y) {t=b; b=c; c=t;}

	/* full triangle */

	dy2 = c->y - a->y;
	if(dy2>0)
	{
		InitLin(&l2[0], a->x, c->x, dy2);
		InitLin(&l2[1], a->r, c->r, dy2);
		InitLin(&l2[2], a->g, c->g, dy2);
		InitLin(&l2[3], a->b, c->b, dy2);

		bline = bmap + w*a->y;

		/* upper part of triangle */

		dy1 = b->y - a->y;
		if(dy1>0)
		{
			InitLin(&l1[0], a->x, b->x, dy1);
			InitLin(&l1[1], a->r, b->r, dy1);
			InitLin(&l1[2], a->g, b->g, dy1);
			InitLin(&l1[3], a->b, b->b, dy1);

			for(y=a->y; y<b->y; y++)
			{
				if(l1[0].v>l2[0].v) DrawLinear(bline,l2,l1);
				else DrawLinear(bline,l1,l2);
				bline += w;
				StepLinArray(l1,0,3,dy1);
				StepLinArray(l2,0,3,dy2);
			}
		}

		/* lower part of triangle */

		dy1 = c->y - b->y;
		if(dy1>0)
		{
			InitLin(&l1[0], b->x, c->x, dy1);
			InitLin(&l1[1], b->r, c->r, dy1);
			InitLin(&l1[2], b->g, c->g, dy1);
			InitLin(&l1[3], b->b, c->b, dy1);

			for(y=b->y; y<c->y; y++)
			{
				if(l1[0].v>l2[0].v) DrawLinear(bline,l2,l1);
				else DrawLinear(bline,l1,l2);
				bline += w;
				StepLinArray(l1,0,3,dy1);
				StepLinArray(l2,0,3,dy2);
			}
		}
	}
}



ULONG ca,cb,cc;

static void DrawLinearDither(ULONG *bline, Lin *l1, Lin *l2)
{
	int x,x1,x2,dx;
	Lin l[LINCHAN];
	int i;
	int pa,pb,pc;
	int r;

	dx = (x2=l2[0].v) - (x1=l1[0].v);
	if(dx>0)
	{
		for(i=1; i<=3; i++)	InitLin(&l[i],l1[i].v, l2[i].v, dx);
		for(x=x1; x<x2; x++)
		{
			pa = l[1].v;
			pb = l[2].v;
			pc = l[3].v;
			r = rand()%(pa+pb+pc);
			if(r<pa) bline[x] = ca;
			else if(r<(pa+pb)) bline[x] = cb;
			else bline[x] = cc;
			StepLinArray(l, 1, 3, dx);
		}
	}
}



static void DrawTriangleDither(ULONG *bmap, int w, int h, Vertex *a, Vertex *b, Vertex *c)
{
	Vertex *t;
	int y;
	int dy1,dy2;
	Lin l1[LINCHAN],l2[LINCHAN];
	ULONG *bline;

	a->pa = 255;	a->pb = 0; a->pc = 0;
	b->pa = 0;	b->pb = 255; b->pc = 0;
	c->pa = 0;	c->pb = 0; c->pc = 255;

	ca = MAKERGB(a->r,a->g,a->b);
	cb = MAKERGB(b->r,b->g,b->b);
	cc = MAKERGB(c->r,c->g,c->b);

	/* y-sort verticles */
	if(a->y>b->y) {t=a; a=b; b=t;}
	if(a->y>c->y) {t=a; a=c; c=t;}
	if(b->y>c->y) {t=b; b=c; c=t;}

	/* full triangle */

	dy2 = c->y - a->y;
	if(dy2>0)
	{
		InitLin(&l2[0], a->x, c->x, dy2);
		InitLin(&l2[1], a->pa, c->pa, dy2);
		InitLin(&l2[2], a->pb, c->pb, dy2);
		InitLin(&l2[3], a->pc, c->pc, dy2);

		bline = bmap + w*a->y;

		/* upper part of triangle */

		dy1 = b->y - a->y;
		if(dy1>0)
		{
			InitLin(&l1[0], a->x, b->x, dy1);
			InitLin(&l1[1], a->pa, b->pa, dy1);
			InitLin(&l1[2], a->pb, b->pb, dy1);
			InitLin(&l1[3], a->pc, b->pc, dy1);

			for(y=a->y; y<b->y; y++)
			{
				if(l1[0].v>l2[0].v) DrawLinearDither(bline,l2,l1);
				else DrawLinearDither(bline,l1,l2);
				bline += w;
				StepLinArray(l1,0,3,dy1);
				StepLinArray(l2,0,3,dy2);
			}
		}

		/* lower part of triangle */

		dy1 = c->y - b->y;
		if(dy1>0)
		{
			InitLin(&l1[0], b->x, c->x, dy1);
			InitLin(&l1[1], b->pa, c->pa, dy1);
			InitLin(&l1[2], b->pb, c->pb, dy1);
			InitLin(&l1[3], b->pc, c->pc, dy1);

			for(y=b->y; y<c->y; y++)
			{
				if(l1[0].v>l2[0].v) DrawLinearDither(bline,l2,l1);
				else DrawLinearDither(bline,l1,l2);
				bline += w;
				StepLinArray(l1,0,3,dy1);
				StepLinArray(l2,0,3,dy2);
			}
		}
	}
}



void HueToRGB(double hue, int *red, int *green, int *blue)
{
	double r,g,b;
	double h;
	double i,f,u;

	h = 360.0*hue;
	h+=90.0;
	if(h>=360.0) h-=360.0;
	h/=60;
	i=floor(h);
	f=h-i; u = 1-f;

	switch((ULONG)i)
	{
		case 0: r=1; g=f; b=0; break;
		case 1: r=u; g=1; b=0; break;
		case 2: r=0; g=1; b=f; break;
		case 3: r=0; g=u; b=1; break;
		case 4: r=f; g=0; b=1; break;
		default: r=1; g=0; b=u; break;
	}
	*red = (int)(r*255.0);
	*green = (int)(g*255.0);
	*blue = (int)(b*255.0);
}


#define TRIANGLES 24 //36
#define BORDER 3

ULONG * __saveds MakeColorWheel(int w, int h,ULONG color0)
{
	ULONG *bmap;
	int a;
	Vertex v[TRIANGLES];
	Vertex vb[TRIANGLES];	// border
	Vertex vc;

	bmap = calloc(h*w,sizeof(ULONG));
	if(!bmap) return NULL;
	for(a=0;a<h*w;a++) bmap[a]=color0;

	/* init verticles */
	for(a=0; a<TRIANGLES; a++)
	{
		v[a].x = (w>>1) + ((w>>1)-BORDER)*cos(a*2*PI/TRIANGLES);
		v[a].y = (h>>1) + ((h>>1)-BORDER)*sin(a*2*PI/TRIANGLES);
		HueToRGB(a/(double)TRIANGLES, &v[a].r, &v[a].g, &v[a].b);
		vb[a].x = (w>>1) + (w>>1)*cos(a*2*PI/TRIANGLES);
		vb[a].y = (h>>1) + (h>>1)*sin(a*2*PI/TRIANGLES);
		vb[a].r = 0;
		vb[a].g = 0;
		vb[a].b = 0;
	}
	/* center vertex */
	vc.x = (w>>1);
	vc.y = (h>>1);
	vc.r = 255;	// White
	vc.g = 255;
	vc.b = 255;

	for(a=0; a<TRIANGLES; a+=1) DrawTriangle(bmap, w, h, &v[a],&v[(a+1)%TRIANGLES],&vc);
	/* set border color to black */
	for(a=0; a<TRIANGLES; a+=1)
	{
		v[a].r = 0;
		v[a].g = 0;
		v[a].b = 0;
	}
	for(a=0; a<TRIANGLES; a+=1)
	{
		DrawTriangle(bmap, w, h, &v[a], &vb[a], &vb[(a+1)%TRIANGLES]);
		DrawTriangle(bmap, w, h, &v[a], &v[(a+1)%TRIANGLES], &vb[(a+1)%TRIANGLES]);
	}

	return bmap;
}



ULONG * __saveds MakeColorWheelDither(int w, int h,ULONG color0)
{
	ULONG *bmap;
	int a;
	Vertex v[TRIANGLES];
	Vertex vs[TRIANGLES];	// center
	Vertex vb[TRIANGLES];	// border
	Vertex vc;

	bmap = calloc(h*w,sizeof(ULONG));
	if(!bmap) return NULL;
	for(a=0;a<h*w;a++) bmap[a]=color0;

	/* init verticles */
	for(a=0; a<TRIANGLES; a++)
	{
		v[a].x = (w>>1) + ((w>>1)-BORDER)*cos(a*2*PI/TRIANGLES);
		v[a].y = (h>>1) + ((h>>1)-BORDER)*sin(a*2*PI/TRIANGLES);
		vs[a].x = (w>>1) + 0.5*((w>>1)-BORDER)*cos(a*2*PI/TRIANGLES);
		vs[a].y = (h>>1) + 0.5*((h>>1)-BORDER)*sin(a*2*PI/TRIANGLES);
		HueToRGB(a/(double)TRIANGLES, &v[a].r, &v[a].g, &v[a].b);
		vs[a].r = (255 + v[a].r)/2;
		vs[a].g = (255 + v[a].g)/2;
		vs[a].b = (255 + v[a].b)/2;
		vb[a].x = (w>>1) + (w>>1)*cos(a*2*PI/TRIANGLES);
		vb[a].y = (h>>1) + (h>>1)*sin(a*2*PI/TRIANGLES);
		vb[a].r = 0;
		vb[a].g = 0;
		vb[a].b = 0;
	}
	/* center vertex */
	vc.x = (w>>1);
	vc.y = (h>>1);
	vc.r = 255;	// White
	vc.g = 255;
	vc.b = 255;

	for(a=0; a<TRIANGLES; a+=1) DrawTriangleDither(bmap, w, h, &vs[a],&vs[(a+1)%TRIANGLES],&vc);
	for(a=0; a<TRIANGLES; a+=1)
	{
		DrawTriangleDither(bmap, w, h, &vs[a],&vs[(a+1)%TRIANGLES],&v[a]);
		DrawTriangleDither(bmap, w, h, &v[a],&v[(a+1)%TRIANGLES],&vs[(a+1)%TRIANGLES]);
	}
	/* set border color to black */
	for(a=0; a<TRIANGLES; a+=1)
	{
		v[a].r = 0;
		v[a].g = 0;
		v[a].b = 0;
	}
	for(a=0; a<TRIANGLES; a+=1)
	{
		DrawTriangleDither(bmap, w, h, &v[a], &vb[a], &vb[(a+1)%TRIANGLES]);
		DrawTriangleDither(bmap, w, h, &v[a], &v[(a+1)%TRIANGLES], &vb[(a+1)%TRIANGLES]);
	}

	return bmap;
}


void __saveds HSToXY(int hue, int sat, int width, int height, UWORD *x, UWORD *y)
{
	*x = (width>>1) + (width>>1)*(sat*cos( 2*PI*hue/255.0 - PI/2 )/256.0);
	*y = (height>>1) + (height>>1)*(sat*sin( 2*PI*hue/255.0 - PI/2)/256.0);
}

