#ifndef __POLYGON_TO_RASTER_H__
#define __POLYGON_TO_RASTER_H__ 1
#include <stdint.h>
#include <stdio.h>



#define CREATE_COLOR(r,g,b) (((r)<<16)|((g)<<8)|((b)))



typedef uint32_t color_t;



typedef struct __POINT{
	uint32_t x;
	uint32_t y;
} point_t;



typedef struct __IMAGE{
	uint8_t* dt;
	uint32_t w;
	uint32_t h;
} image_t;



void create_image(uint32_t w,uint32_t h,image_t* o);



void polygon_to_raster(const point_t* p,uint32_t pl,image_t* o);



void image_to_file(const image_t* img,color_t a,color_t b,FILE* o);



void free_image(image_t* img);



#endif
