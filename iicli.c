#include <stdio.h>
#include <string.h>

#define ISL_IMAGE_INFO_IMPLEMENTATION
#include "isl_image_info.h"

int main( int argc, char* argv[] ) {
	int status = 0;
	if ( argc < 2 || !strcmp( argv[1], "--help" ) || !strcmp( argv[1], "-?" )) {
		printf( "Command line utility to get basic info about image file.\n" );
		printf( "Usage:\n  iicli <image file>\n" );
		printf( "Supported file formats are: PNG, JPEG, GIF, BMP\n" );
	 	printf( "Coded by Ilya Kolbin (iskolbin@gmail.com) in 2016\n" );
		status = 1;
	} else {
		FILE *f = fopen( argv[1], "r" );
		islii_result result = islii_info_file( f );
	 	if ( result.error == ISLII_ERROR_OK ) {
			printf( "width:%ld height:%ld format:%s bpp:%d colortype:%s", result.width, result.height, islii_strformat( result.format ), result.bpp, islii_strcolortype( result.colortype ));
		} else {
			printf( "Error code: %d, str: %s", result.error, islii_strerror( result.error ));
			status = 2;
		}
		fclose( f );
	}
	return status;
}
