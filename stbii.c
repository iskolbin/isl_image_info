#include <stdio.h>
#include <string.h>
#include "stb_image_info.h"

int main( int argc, char* argv[] ) {
	if ( argc < 2 || !strcmp( argv[1], "--help" ) || !strcmp( argv[1], "-?" )) {
		printf( "Command line utility to get basic info about image file.\n" );
		printf( "Usage:\n  stbii <image file>\n" );
		printf( "Returns width, height and file extension separated by space\n" );
		printf( "Supported file formats are: PNG, JPEG, GIF, BMP and TIFF\n" );
	 	printf( "Coded by Ilya Kolbin (iskolbin@gmail.com) in 2016\n" );	
	} else {
		unsigned long int width;
		unsigned long int height;
		stbii_format format;
		stbii_result result = stbii_info( argv[1], &width, &height, &format );
	 	if ( result	== STBII_RESULT_OK ) {
			printf( "%ld %ld %s", width, height, stbii_strformat( format )); 
		} else {
			printf( "Error code: %d, str: %s", result, stbii_strresult( result ));	
		}
	}
	return 0;
}
