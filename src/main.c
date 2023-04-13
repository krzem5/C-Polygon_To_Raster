#include <polygon_to_raster.h>
#include <stdio.h>



int main(int argc,const char** argv){
	const point_t p[]={
		{10,10},
		{25,10},
		{20,20},
		{10,25}
	};
	image_t img;
	create_image(32,32,&img);
	polygon_to_raster(p,4,&img);
	FILE* f=fopen("export.bmp","wb");
	image_to_file(&img,CREATE_COLOR(0,0,0),CREATE_COLOR(255,255,255),f);
	fclose(f);
	free_image(&img);
	return 0;
}
