#ifndef STBII_INCLUDE_STB_IMAGE_INFO_H
#define STBII_INCLUDE_STB_IMAGE_INFO_H

#include <stdio.h>
#include <stdlib.h>

typedef enum {
	STBII_UNKNOWN,
	STBII_BMP,
	STBII_PNG,
	STBII_JPEG,
	STBII_GIF,
	STBII_TIFF,
} stbii_format;

typedef enum {
	STBII_RESULT_OK,
	STBII_RESULT_UNKNOWN_FORMAT,
	STBII_RESULT_FILE_OPEN_ERROR,
	STBII_RESULT_BAD_JPEG_MARKER,
	STBII_RESULT_BAD_TIFF,
} stbii_result;

#ifdef __cplusplus
extern "C" {
#endif

extern stbii_result stbii_info( const char *filename, unsigned long int *pwidth, unsigned long int *pheight, stbii_format *pformat );
extern const char *stbii_strformat( stbii_format format );

#ifdef __cplusplus
}
#endif

static unsigned int fread_u16_little( FILE *f ) {
	unsigned char a, b;
	fread( &a, 1, 1, f );
	fread( &b, 1, 1, f );
	return (b<<8) + a;
}

static unsigned int fread_u32_little( FILE * f ) {
	unsigned char a, b, c, d;
	fread( &a, 1, 1, f );
	fread( &b, 1, 1, f );
	fread( &c, 1, 1, f );
	fread( &d, 1, 1, f );
	return (d<<24) + (c<<16) + (b<<8) + a;
}

static unsigned int fread_u16_big( FILE *f ) {
	unsigned char a, b;
	fread( &a, 1, 1, f );
	fread( &b, 1, 1, f );
	return (a<<8) + b;
}

static unsigned long int fread_u32_big( FILE *f ) {
	unsigned char a, b, c, d;
	fread( &a, 1, 1, f );
	fread( &b, 1, 1, f );
	fread( &c, 1, 1, f );
	fread( &d, 1, 1, f );
	return (a<<24) + (b<<16) + (c<<8) + d;
}

static unsigned int fread_u16( FILE *f, int big ) {
	if ( big ) {
		return fread_u16_big( f );
	} else {
		return fread_u16_little( f );
	}
}

static unsigned long int fread_u32( FILE *f, int big ) {
	if ( big ) {
		return fread_u32_big( f );
	} else {
		return fread_u32_little( f );
	}
}

static stbii_result fread_png( FILE *f, unsigned long int *width, unsigned long int *height, stbii_format *format ) {
	unsigned char third;
	fread( &third, 1, 1, f );
	if ( third == 78 ) {
		fseek( f, 13, SEEK_CUR );
		*width = fread_u32_big( f );
		*height = fread_u32_big( f );
		*format = STBII_PNG;
		return STBII_RESULT_OK;
	} else {
		return STBII_RESULT_UNKNOWN_FORMAT;
	}
}

static stbii_result fread_bmp( FILE *f, unsigned long int *width, unsigned long int *height, stbii_format *format ) {
	fseek( f, 16, SEEK_CUR );
	*width = fread_u32_little( f );
	*height = fread_u32_little( f );
	*format = STBII_BMP;
	return STBII_RESULT_OK;
}

static stbii_result fread_gif( FILE *f, unsigned long int *width, unsigned long int *height, stbii_format *format ) {
	unsigned char third;
	fread( &third, 1, 1, f );
	if ( third == 'F' ) {
		fseek( f, 3, SEEK_CUR );			
		*width = fread_u16_little( f );
		*height = fread_u16_little( f );
		*format = STBII_GIF;

		return STBII_RESULT_OK;
	} else {
		return STBII_RESULT_UNKNOWN_FORMAT;
	}
}

static stbii_result fread_jpeg( FILE *f, unsigned long int *width, unsigned long int *height, stbii_format *format ) {
	*format = STBII_JPEG;
	for(;;) switch (fread_u16_big( f )) {
		case 0xffd8: case 0xffd0: case 0xffd1: case 0xffd2: case 0xffd3:
		case 0xffd4: case 0xffd5: case 0xffd6: case 0xffd7: case 0xffd9:
			break;

		case 0xffdd:
			fread_u16_big( f );
			break;

		case 0xffe0: case 0xffe1: case 0xffe2: case 0xffe3: case 0xffe4:
		case 0xffe5: case 0xffe6: case 0xffe7: case 0xffe8: case 0xffe9: 
		case 0xffea: case 0xffeb: case 0xffec: case 0xffed: case 0xffee:
		case 0xffef: case 0xfffe: case 0xffdb: case 0xffc4: case 0xffda: 
			fseek( f, ((long int) fread_u16_big( f )) - 2, SEEK_CUR );
			break;

		case 0xffc0: case 0xffc2:
			fseek( f, 3, SEEK_CUR );
			*width = fread_u16_big( f );
			*height = fread_u16_big( f );
			return STBII_RESULT_OK;

		default:
			return STBII_RESULT_BAD_JPEG_MARKER;
	}
}

static stbii_result fread_tiff( FILE *f, unsigned long int *width, unsigned long int *height, stbii_format *format, int big ) {
	int found = 0;
	unsigned int i;
	unsigned int entries;
	unsigned int rest = fread_u16_big( f );

	if ((big && rest != 0x2A) || (!big && rest != 0x2A00)) {
		return STBII_RESULT_UNKNOWN_FORMAT;
	}
	
	*format = STBII_TIFF;
	fseek( f, fread_u32( f, big ) - 8, SEEK_CUR );
	entries = fread_u16( f, big );
	for ( i = 1; i <= entries && found != 3; i++ ) {
		unsigned int tag = fread_u16( f, big );
		unsigned int fieldType = fread_u16( f, big );
		unsigned long int count = fread_u32( f, big );
		unsigned int offset;
		
		if (( fieldType == 3 || fieldType == 8 )) {
			offset = fread_u16( f, big );
			fseek( f, 2, SEEK_CUR );
		} else {
			offset = fread_u16( f, big );
		}

		if ( tag == 0x100 ) {
			*width = offset;
			found |= 1;
		} else if ( tag == 0x101 ) {
			*height = offset;
			found |= 2;
		}
	}

	if ( found == 3 ) {
		return STBII_RESULT_OK;
	} else {
		return STBII_RESULT_BAD_TIFF;
	}
}

stbii_result stbii_info( const char *filename, unsigned long int *pwidth, unsigned long int *pheight, stbii_format *pformat ) {
	FILE *f = fopen( filename, "r" );
	unsigned long int width = 0;
	unsigned long int height = 0;
	stbii_format format = STBII_UNKNOWN;
	stbii_result result = STBII_RESULT_OK;

	if ( !f ) {
		result = STBII_RESULT_FILE_OPEN_ERROR;
	} else {
		unsigned int header = fread_u16_big( f );
		switch ( header ) {
			case 0x8950: result = fread_png( f, &width, &height, &format ); break;
			case 0x424D: result = fread_bmp( f, &width, &height, &format ); break;
			case 0x4749: result = fread_gif( f, &width, &height, &format ); break;
			case 0xFFD8: result = fread_jpeg( f, &width, &height, &format ); break;
			case 0x4D4D: result = fread_tiff( f, &width, &height, &format, 1 ); break;
			case 0x4949: result = fread_tiff( f, &width, &height, &format, 0 ); break;
			default: result = STBII_RESULT_UNKNOWN_FORMAT;
		}
	}

	if ( result == STBII_RESULT_OK ) {
		if ( pwidth != NULL ) *pwidth = width;
		if ( pheight != NULL ) *pheight = height;
		if ( pformat != NULL ) *pformat = format;
	}

	return result;
}

const char *stbii_strformat( stbii_format format ) {
	switch ( format ) {
		case STBII_BMP: return "BMP"; break;
		case STBII_PNG: return "PNG"; break;
		case STBII_JPEG: return "JPEG"; break;
		case STBII_GIF: return "GIF"; break;
		case STBII_TIFF: return "TIFF"; break;
		default: return "UNKNOWN";
	}
}

#endif
