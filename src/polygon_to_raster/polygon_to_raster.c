#include <polygon_to_raster.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>



#define _ASSERT_STRINGIFY_(x) #x
#define _ASSERT_STRINGIFY(x) _ASSERT_STRINGIFY_(x)
#define ASSERT(x) \
	do{ \
		if (!(x)){ \
			printf("File \""__FILE__"\", Line "_ASSERT_STRINGIFY(__LINE__)" (%s): "_ASSERT_STRINGIFY(x)": Assertion Failed\n",__func__); \
			raise(SIGABRT); \
		} \
	} while (0)

#define BMP_HEADER_SIZE 54



typedef struct __LINE_COORD{
	uint32_t x0;
	uint32_t x1;
} line_coord_t;



typedef struct __LINE_SLOPE{
	float a;
	float b;
} line_slope_t;



typedef union __LINE_DATA{
	line_coord_t c;
	line_slope_t s;
} line_data_t;



typedef struct __LINE{
	uint32_t y0;
	uint32_t y1;
	line_data_t dt;
} line_t;



static uint32_t _sort_partition(line_t* a,uint32_t i,uint32_t j){
	uint32_t p=(a+j)->dt.c.x0;
	uint32_t o=i;
	while (i<j){
		if ((a+i)->dt.c.x0<p||(a+i)->dt.c.x1<p){
			line_t t=*(a+o);
			*(a+o)=*(a+i);
			*(a+i)=t;
			o++;
		}
		i++;
	}
	line_t t=*(a+o);
	*(a+o)=*(a+i);
	*(a+i)=t;
	return o;
}



static void _sort(line_t* a,uint32_t i,uint32_t j){
	if (i<j){
		uint32_t k=_sort_partition(a,i,j);
		if (k){
			_sort(a,i,k-1);
		}
		_sort(a,k+1,j);
	}
}



void create_image(uint32_t w,uint32_t h,image_t* o){
	ASSERT(!(w&7));
	o->w=w;
	o->h=h;
	o->dt=calloc(sizeof(uint8_t),(o->h*o->w)>>3);
}



void polygon_to_raster(const point_t* p,uint32_t pl,image_t* o){
	ASSERT(pl>2);
	uint32_t bbox[4]={p[0].x,p[0].y,p[0].x,p[0].y};
	line_t* l=malloc(pl*sizeof(line_t));
	if (p[0].y<p[pl-1].y){
		l->dt.c.x0=p[0].x;
		l->y0=p[0].y;
		l->dt.c.x1=p[pl-1].x;
		l->y1=p[pl-1].y;
	}
	else{
		l->dt.c.x0=p[pl-1].x;
		l->y0=p[pl-1].y;
		l->dt.c.x1=p[0].x;
		l->y1=p[0].y;
	}
	for (uint32_t i=1;i<pl;i++){
		if (p[i].x<bbox[0]){
			bbox[0]=p[i].x;
		}
		if (p[i].y<bbox[1]){
			bbox[1]=p[i].y;
		}
		if (p[i].x>=bbox[2]){
			bbox[2]=p[i].x+1;
		}
		if (p[i].y>=bbox[3]){
			bbox[3]=p[i].y+1;
		}
		if (p[i-1].y<p[i].y){
			(l+i)->dt.c.x0=p[i-1].x;
			(l+i)->y0=p[i-1].y;
			(l+i)->dt.c.x1=p[i].x;
			(l+i)->y1=p[i].y;
		}
		else{
			(l+i)->dt.c.x0=p[i].x;
			(l+i)->y0=p[i].y;
			(l+i)->dt.c.x1=p[i-1].x;
			(l+i)->y1=p[i-1].y;
		}
	}
	_sort(l,0,pl-1);
	for (uint32_t i=0;i<pl;i++){
		float x0=(float)((l+i)->dt.c.x0);
		float x1=(float)((l+i)->dt.c.x1);
		(l+i)->dt.s.a=((l+i)->y0==(l+i)->y1?1:(x1-x0)/((float)((l+i)->y1)-(l+i)->y0));
		(l+i)->dt.s.b=x0-(!(l+i)->dt.s.a?0:((float)(l+i)->y0)*(l+i)->dt.s.a);
	}
	for (uint32_t i=bbox[1];i<bbox[3];i++){
		uint32_t off=i*(o->w>>3);
		uint32_t j=0xffffffff;
		uint8_t st=0;
		for (uint32_t k=0;k<pl;k++){
			if (i<(l+k)->y0||i>(l+k)->y1){
				continue;
			}
			uint32_t x=(uint32_t)roundf(i*(l+k)->dt.s.a+(l+k)->dt.s.b);
			if (x<bbox[0]&&x>=bbox[2]){
				continue;
			}
			if (j==x){
				o->dt[off+(j>>3)]|=st<<(7-(j&7));
			}
			else{
				if (st){
					uint32_t n=x;
					if (j>n){
						uint32_t t=n;
						n=j;
						j=t;
					}
					n++;
					if (j>>3==n>>3){
						o->dt[off+(j>>3)]|=((1<<(n-j))-1)<<((j&0xfffffff8)-n+8);
					}
					else{
						uint8_t m=(1<<(8-(j&7)))-1;
						j>>=3;
						o->dt[off+j]|=m;
						m=0xff<<(8-(n&7));
						n>>=3;
						o->dt[off+n]|=m;
						j++;
						while (j<n){
							o->dt[off+j]=0xff;
							j++;
						}
					}
				}
				j=x;
				st=!st;
			}
		}
	}
	free(l);
}



void image_to_file(const image_t* img,color_t a,color_t b,FILE* o){
	ASSERT(img);
	ASSERT(o);
	ASSERT(BMP_HEADER_SIZE<256);
	uint32_t dt_sz=(((img->w+7)&0xfffffff8)*img->h)<<3;
	uint32_t sz=BMP_HEADER_SIZE+dt_sz;
	uint8_t bm_h[BMP_HEADER_SIZE+8]={'B','M',sz&0xff,(sz>>8)&0xff,(sz>>16)&0xff,sz>>24,0,0,0,0,BMP_HEADER_SIZE+8,0,0,0,40,0,0,0,img->w&0xff,(img->w>>8)&0xff,(img->w>>16)&0xff,img->w>>24,img->h&0xff,(img->h>>8)&0xff,(img->h>>16)&0xff,img->h>>24,1,0,1,0,0,0,0,0,dt_sz&0xff,(dt_sz>>8)&0xff,(dt_sz>>16)&0xff,dt_sz>>24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,a&0xff,(a>>8)&0xff,(a>>16)&0xff,0,b&0xff,(b>>8)&0xff,(b>>16)&0xff,0};
	ASSERT(fwrite(bm_h,sizeof(uint8_t),BMP_HEADER_SIZE+8,o)==BMP_HEADER_SIZE+8);
	uint32_t i=img->h;
	uint32_t w=img->w>>3;
	while (i){
		i--;
		fwrite(img->dt+i*w,sizeof(uint8_t),w,o);
	}
}



void free_image(image_t* img){
	free(img->dt);
	img->dt=NULL;
	img->w=0;
	img->h=0;
}
